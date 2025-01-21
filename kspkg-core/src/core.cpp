#include <kspkg-core/core.hpp>

namespace kspkg {
    constexpr size_t kXorKey = 0x9F9721A97D1135C1;
    constexpr size_t kMetadataSize = 0x2000000;

    namespace detail {

        inline void encrypt_decrypt_data( std::vector< uint8_t >& data, size_t key ) {
            size_t i = 0;

            for ( ; i + sizeof( size_t ) <= data.size(); i += sizeof( size_t ) ) {
                *reinterpret_cast< size_t* >( &data[ i ] ) ^= key;
            }

            for ( ; i < data.size(); ++i ) {
                data[ i ] ^= static_cast< uint8_t >( key );
                key >>= 8;
            }
        }

    } // namespace detail

    expected< bool > package::extract_file( const std::shared_ptr< file >& file, const std::filesystem::path& out_directory ) {
        stream_.seekg( static_cast< std::streamoff >( file->get_file_offset() ), std::ios::beg );

        const std::filesystem::path out_path = std::filesystem::path( out_directory ) / file->get_name();
        create_directories( std::filesystem::path( out_path ).parent_path() );

        if ( file->is_directory() ) {
            return unexpected( "Cannot extract a directory." );
        }

        if ( !file->is_file() ) {
            return unexpected( "Cannot extract unknown file." );
        }

        std::vector< uint8_t > data( file->get_file_size() );
        stream_.read( reinterpret_cast< char* >( data.data() ), static_cast< std::streamsize >( file->get_file_size() ) );
        detail::encrypt_decrypt_data( data, kXorKey );

        std::ofstream output( out_path, std::ios::binary );
        output.write( reinterpret_cast< char* >( data.data() ), static_cast< std::streamsize >( file->get_file_size() ) );

        return true;
    }

    expected< std::vector< uint8_t > > package::extract_file( const std::shared_ptr< file >& file ) {
        std::vector< uint8_t > result;

        stream_.seekg( static_cast< std::streamoff >( file->get_file_offset() ), std::ios::beg );

        if ( file->is_directory() ) {
            return unexpected( "Cannot extract a directory." );
        }

        result.resize( file->get_file_size() );
        stream_.read( reinterpret_cast< char* >( result.data() ), static_cast< std::streamsize >( file->get_file_size() ) );

        if ( file->is_file() ) {
            detail::encrypt_decrypt_data( result, kXorKey );
        }

        return result;
    }

    expected< std::shared_ptr< package > > load_package( const std::filesystem::path& path ) {
        std::ifstream fs( path, std::ios::binary );
        if ( !fs.is_open() )
            return unexpected( "Failed to open the package file for reading." );

        fs.seekg( -static_cast< std::streamoff >( kMetadataSize ), std::ios::end );

        std::vector< uint8_t > metadata_buffer( kMetadataSize );
        fs.read( reinterpret_cast< char* >( metadata_buffer.data() ), kMetadataSize );
        detail::encrypt_decrypt_data( metadata_buffer, kXorKey );

        std::vector< std::shared_ptr< file > > files;

        constexpr auto max_count = kMetadataSize / sizeof( file_desc_t );
        for ( size_t i = 0; i < max_count; i++ ) {
            if ( const auto& file_desc = *reinterpret_cast< file_desc_t* >( metadata_buffer.data() + i * sizeof( file_desc_t ) );
                 file_desc.file_size != 0 ) {
                files.emplace_back( std::make_shared< file >( file_desc ) );
            }
        }

        return std::make_shared< package >( std::move( fs ), path, files );
    }

    expected< void > repack_package( const std::shared_ptr< package >& package, const std::vector< std::filesystem::path >& new_filespathes,
                                     const std::filesystem::path& new_files_root_dir ) {
        std::ofstream fs( package->get_path(), std::ios::binary | std::ios::app );
        if ( !fs.is_open() )
            return unexpected( "Failed to open the package file for writing." );

        const auto& files = package->get_files();

        // Write `marker` to find it when removing patch
        const std::vector< uint8_t > marker = { 0x31, 0x32, 0x33, 0x34, 0x35 };
        fs.write( reinterpret_cast< const char* >( marker.data() ), static_cast< std::streamsize >( marker.size() ) );

        for ( const auto& new_filepath : new_filespathes ) {
            std::ifstream file( new_filepath, std::ios::binary );
            if ( !file.is_open() )
                continue;

            file.seekg( 0, std::ios::end );
            const auto file_size = file.tellg();
            file.seekg( 0, std::ios::beg );

            std::vector< uint8_t > new_data( file_size );
            file.read( reinterpret_cast< char* >( new_data.data() ), file_size );

            detail::encrypt_decrypt_data( new_data, kXorKey );

            fs.seekp( 0, std::ios::end );
            const auto offset = fs.tellp();
            fs.seekp( 0, std::ios::beg );

            const auto virtual_full_filename = new_files_root_dir / new_filepath.filename().string();

            // Find file in package and overwrite its desc
            for ( auto&& loaded_file : files ) {
                if ( loaded_file->get_name() == virtual_full_filename ) {
                    auto& desc = loaded_file->desc();
                    desc.file_size = file_size;
                    desc.file_offset = offset;
                    fs.write( reinterpret_cast< const char* >( new_data.data() ), static_cast< std::streamsize >( new_data.size() ) );
                    break;
                }
            }
        }

        // Just push metadata to the end of the file without care about old metadata
        std::vector< uint8_t > metadata( kMetadataSize );
        for ( size_t i = 0; i < files.size(); i++ ) {
            const auto& file = files[ i ];
            auto* desc = reinterpret_cast< file_desc_t* >( metadata.data() + i * sizeof( file_desc_t ) );
            *desc = file->desc();
        }

        detail::encrypt_decrypt_data( metadata, kXorKey );

        fs.write( reinterpret_cast< const char* >( metadata.data() ), static_cast< std::streamsize >( metadata.size() ) );

        return {};
    }

    expected< bool > remove_patches( const std::shared_ptr< package >& package ) {
        const std::vector< uint8_t > marker = { 0x31, 0x32, 0x33, 0x34, 0x35 };
        const auto& package_path = package->get_path();

        std::ifstream fs_in( package_path, std::ios::binary );
        if ( !fs_in.is_open() ) {
            return unexpected( "Failed to open the package file for reading." );
        }

        // Define chunk size for reading in reverse
        constexpr size_t chunk_size = 0x10000000; // 256 MB
        std::vector< uint8_t > buffer( chunk_size );

        fs_in.seekg( 0, std::ios::end );
        const auto file_size = fs_in.tellg();

        size_t read_position = file_size;
        size_t marker_position = std::string::npos;

        while ( read_position > 0 ) {
            const size_t read_amount = std::min( chunk_size, read_position );
            read_position -= read_amount;

            fs_in.seekg( static_cast< std::streamoff >( read_position ), std::ios::beg );
            fs_in.read( reinterpret_cast< char* >( buffer.data() ), static_cast< std::streamsize >( read_amount ) );

            if ( auto it = std::search( buffer.rbegin(), buffer.rend(), marker.rbegin(), marker.rend() ); it != buffer.rend() ) {
                marker_position = read_position + ( buffer.rend() - it - marker.size() );
                break;
            }

            // TODO: Make smarter system
            // Drop search when marker is not found in the first chunk (256 MB)
            break;
        }

        fs_in.close();

        if ( marker_position == std::string::npos ) {
            return false;
        }

        std::ofstream fs_out( package_path, std::ios::binary | std::ios::in );
        if ( !fs_out.is_open() ) {
            return unexpected( "Failed to open the package file for writing." );
        }
        fs_out.seekp( static_cast< std::streamoff >( marker_position ), std::ios::beg );
        fs_out.close();

        resize_file( package_path, marker_position );

        return true;
    }

} // namespace kspkg