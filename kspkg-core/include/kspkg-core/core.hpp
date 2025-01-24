#pragma once

#include <filesystem>
#include <memory>
#include <span>
#include <fstream>
#include <string>
#include <vector>
#include <string_view>
#include <expected>

namespace kspkg {

    template < typename T >
    using expected = std::expected< T, std::string >;
    using unexpected = std::unexpected< std::string >;

    enum class file_flags_t : uint16_t {
        kIsDirectory = 1 << 0,
        kIsEncrypted = 1 << 8, // https://github.com/ntpopgetdope/ace-kspkg
    };

    struct file_desc_t {
        char name[ 0xE0 ] {};
        uint8_t gap0[ 0x4 ] {};
        uint16_t flags;
        uint16_t name_length;
        uint8_t gap1[ 0x8 ] {};
        size_t file_size;
        size_t file_offset;
    };

    class file {
    public:
        file() = default;
        explicit file( const file_desc_t& desc ) : desc_( desc ) { }

        [[nodiscard]] std::string_view get_name() const noexcept {
            return { desc_.name, desc_.name_length };
        }

        [[nodiscard]] bool is_directory() const noexcept {
            return desc_.flags & static_cast< uint16_t >( file_flags_t::kIsDirectory );
        }

        [[nodiscard]] bool is_encrypted() const noexcept {
            return desc_.flags & static_cast< uint16_t >( file_flags_t::kIsEncrypted );
        }

        [[nodiscard]] size_t get_file_size() const noexcept {
            return desc_.file_size;
        }

        [[nodiscard]] size_t get_file_offset() const noexcept {
            return desc_.file_offset;
        }

        [[nodiscard]] file_desc_t& desc() noexcept {
            return desc_;
        }

    private:
        file_desc_t desc_;
    };

    class package {
    public:
        package() = default;
        explicit package( std::ifstream stream, const std::filesystem::path& path, const std::vector< std::shared_ptr< file > >& files )
            : stream_( std::move( stream ) ), path_( path ), files_( files ) { }

        [[nodiscard]] std::string get_name() const noexcept {
            return path_.filename().string();
        }

        [[nodiscard]] const std::filesystem::path& get_path() const noexcept {
            return path_;
        }

        [[nodiscard]] const std::vector< std::shared_ptr< file > >& get_files() const noexcept {
            return files_;
        }

        /**
         * @brief Extract file from the package
         * @param file File to extract
         * @param out_directory Directory to extract the file
         */
        expected< bool > extract_file( const std::shared_ptr< file >& file, const std::filesystem::path& out_directory );

        /**
         * @brief Extract file from the package
         * @param file Extracted file
         */
        expected< std::vector< uint8_t > > extract_file( const std::shared_ptr< file >& file );

    private:
        std::ifstream stream_;
        std::filesystem::path path_;
        std::vector< std::shared_ptr< file > > files_;
    };

    /**
     * @brief Load package from the file
     * @param path Path to the package file
     * @return Loaded package
     */
    expected< std::shared_ptr< package > > load_package( const std::filesystem::path& path );

    /**
     * @brief Repack package with new files
     * @param package Package to repack
     * @param new_filespathes Pathes to the new files
     * @param new_files_root_dir Virtual root directory for the files
     */
    expected< void > repack_package( const std::shared_ptr< package >& package, const std::vector< std::filesystem::path >& new_filespathes,
                                     const std::filesystem::path& new_files_root_dir );

    /**
     * @brief Remove patches from the package
     * @param package Package to remove patches
     */
    expected< bool > remove_patches( const std::shared_ptr< package >& package );
} // namespace kspkg