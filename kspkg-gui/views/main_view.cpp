#include "main_view.hpp"

#include <Windows.h>
#include <imgui.h>
#include <kspkg-core/include.hpp>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <ranges>

namespace views {

    void build_hierarchy( const std::vector< std::shared_ptr< kspkg::file > >& files, const std::shared_ptr< file_node_t >& root ) {
        for ( const auto& file : files ) {
            auto current_node = root;
            size_t start = 0, end;

            const std::string path = file->get_name().data();
            while ( ( end = path.find_first_of( "/\\", start ) ) != std::string::npos ) {
                if ( auto folder_name = path.substr( start, end - start ); !folder_name.empty() ) {
                    if ( !current_node->children.contains( folder_name ) ) {
                        current_node->children[ folder_name.data() ] = std::make_shared< file_node_t >();
                    }
                    current_node = current_node->children[ folder_name.data() ];
                }
                start = end + 1;
            }

            if ( auto file_name = path.substr( start ); !file_name.empty() ) {
                current_node->children[ file_name.data() ] = std::make_shared< file_node_t >();
                current_node->children[ file_name.data() ]->is_file = true;
                current_node->children[ file_name.data() ]->name = file_name;
                current_node->children[ file_name.data() ]->ref = file;
            }
        }
    }

    bool has_matching_files( const std::shared_ptr< file_node_t >& node, const std::string& filter ) {
        if ( node->is_file ) {
            return filter.empty() || node->name.find( filter ) != std::string::npos;
        }

        for ( const auto& child_node : node->children | std::views::values ) {
            if ( has_matching_files( child_node, filter ) ) {
                return true;
            }
        }

        return false;
    }

    void render_hierarchy( const std::shared_ptr< file_node_t >& node, const std::string& name, const std::string& filter,
                           std::shared_ptr< file_node_t >& selected_node ) {
        if ( !has_matching_files( node, filter ) ) {
            return;
        }

        constexpr auto folder_color = ImVec4( 0.5f, 0.8f, 1.0f, 1.0f );
        constexpr auto file_color = ImVec4( 0.8f, 0.8f, 0.8f, 1.0f );

        if ( node->is_file ) {
            ImGui::PushStyleColor( ImGuiCol_Text, file_color );
            if ( ImGui::Selectable( name.c_str(), selected_node == node, 0, { 400.f, 0.f } ) ) {
                selected_node = node;
            }
            ImGui::PopStyleColor();
        }
        else {
            ImGui::PushStyleColor( ImGuiCol_Text, folder_color );
            if ( ImGui::TreeNode( name.c_str() ) ) {
                ImGui::PopStyleColor();

                std::vector< std::pair< std::string, std::shared_ptr< file_node_t > > > directories;
                std::vector< std::pair< std::string, std::shared_ptr< file_node_t > > > files;

                for ( const auto& [ child_name, child_node ] : node->children ) {
                    if ( child_node->is_file ) {
                        files.emplace_back( child_name, child_node );
                    }
                    else {
                        directories.emplace_back( child_name, child_node );
                    }
                }

                auto sort_by_name = []( const auto& a, const auto& b ) {
                    return a.first < b.first;
                };

                std::ranges::sort( directories, sort_by_name );
                std::ranges::sort( files, sort_by_name );

                for ( const auto& [ child_name, child_node ] : directories ) {
                    render_hierarchy( child_node, child_name, filter, selected_node );
                }

                for ( const auto& [ child_name, child_node ] : files ) {
                    render_hierarchy( child_node, child_name, filter, selected_node );
                }

                ImGui::TreePop();
            }
            else {
                ImGui::PopStyleColor();
            }
        }
    }

    bool has_extension( const std::string& file_name, const std::vector< std::string >& extensions ) {
        for ( const auto& ext : extensions ) {
            if ( file_name.size() >= ext.size() && file_name.compare( file_name.size() - ext.size(), ext.size(), ext ) == 0 ) {
                return true;
            }
        }
        return false;
    }

    void main_view::render() {
        static char filter_buffer[ 256 ] = "";
        static std::shared_ptr< file_node_t > selected_node = nullptr;

        if ( package_ == nullptr ) {
            const auto content_path = std::filesystem::current_path() / ".." / "content.kspkg";
            if ( const auto package = kspkg::load_package( content_path ); package.has_value() ) {
                package_ = package.value();
                root_node_ = std::make_shared< file_node_t >();

                std::vector< std::shared_ptr< kspkg::file > > file_paths;
                for ( const auto& file : package_->get_files() ) {
                    if ( !file->is_directory() ) {
                        file_paths.emplace_back( file );
                    }
                }

                build_hierarchy( file_paths, root_node_ );
            }
            else {
                MessageBoxA( nullptr, "Failed to find content.kspkg. Make sure you moved `kspkg` folder to ACE root folder.", "Error",
                             MB_ICONERROR );
                ExitProcess( 0 );
            }
        }

        const auto viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( viewport->Pos );
        ImGui::SetNextWindowSize( viewport->Size );

        ImGui::Begin( "Main View", nullptr,
                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                          ImGuiWindowFlags_MenuBar );

        ImGui::BeginMenuBar();

        if ( ImGui::BeginMenu( "Addons" ) ) {
            if ( ImGui::MenuItem( "Install Russian Language", nullptr, nullptr, true ) ) {
                const auto lang_dir = std::filesystem::current_path() / "resources" / "ru_lang";
                if ( const auto result = repack_package( package_,
                                                         {
                                                             lang_dir / "cn.cars.loc",
                                                             lang_dir / "cn.loc",
                                                             lang_dir / "cn.tooltips.loc",
                                                         },
                                                         R"(uiresources\localization\)" ) ) {
                    if ( *result ) {
                        MessageBoxA( nullptr, "Russian language installed successfully.", "Install Russian Language", MB_ICONINFORMATION );
                    }
                    else {
                        MessageBoxA( nullptr, "Unexpected", "Install Russian Language", MB_ICONERROR );
                    }
                }
                else {
                    MessageBoxA( nullptr, result.error().data(), "Install Russian Language", MB_ICONERROR );
                }
            }
            if ( ImGui::MenuItem( "Remove All Patches", nullptr, nullptr, true ) ) {
                if ( const auto result = remove_patches( package_ ) ) {
                    if ( *result ) {
                        MessageBoxA( nullptr, "All patches removed successfully.", "Remove Patches", MB_ICONINFORMATION );
                    }
                    else {
                        MessageBoxA( nullptr, "No patches found.", "Remove Patches", MB_ICONWARNING );
                    }
                }
                else {
                    MessageBoxA( nullptr, result.error().data(), "Remove Patches", MB_ICONERROR );
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();

        ImGui::Columns( 2, "HierarchyPreview" );

        // Left Column
        {
            ImGui::Text( "Filter:" );
            ImGui::InputText( "##FilterInput", filter_buffer, sizeof( filter_buffer ) );

            ImGui::Text( "Items:" );
            ImGui::BeginChild( "LeftPanel", ImVec2( 0, 0 ), 0, 0 );

            if ( root_node_ ) {
                for ( const auto& [ child_name, child_node ] : root_node_->children ) {
                    render_hierarchy( child_node, child_name, filter_buffer, selected_node );
                }
            }

            ImGui::EndChild();
        }

        ImGui::NextColumn();

        // Right Column - Preview
        {
            ImGui::Text( "Preview:" );

            if ( selected_node ) {
                if ( selected_node->is_file ) {
                    ImGui::Text( "File: %s", selected_node->name.c_str() );

                    ImGui::SameLine();

                    if ( ImGui::Button( "Extract" ) ) {
                        const auto out_path = std::filesystem::current_path() / "__output__";
                        if ( const auto file = package_->extract_file( selected_node->ref, out_path ); !file ) {
                            MessageBoxA( nullptr, file.error().data(), "Extraction Error", MB_ICONERROR );
                        }
                    }

                    static const std::vector< std::string > kTextExtensions = { ".txt", ".ini", ".json", ".html", ".css", ".js", ".loc" };

                    if ( has_extension( selected_node->name, kTextExtensions ) ) {
                        if ( const auto file_content = package_->extract_file( selected_node->ref ) ) {
                            ImGui::PushStyleColor( ImGuiCol_ChildBg, { 0.06f, 0.06f, 0.06f, 1.f } );
                            ImGui::BeginChild( "FilePreview", ImVec2( 0, 0 ), 0, ImGuiWindowFlags_HorizontalScrollbar );

                            const std::string file_content_str( file_content->data(), file_content->data() + file_content->size() );
                            ImGui::TextUnformatted( file_content_str.data(), file_content_str.data() + file_content_str.size() );

                            ImGui::EndChild();
                            ImGui::PopStyleColor();
                        }
                    }
                    else {
                        ImGui::Text( "Unsupported file type for preview." );
                    }
                }
                else {
                    ImGui::Text( "Folder: %s", selected_node->name.c_str() );
                    ImGui::Text( "Select a file to preview its content." );
                }
            }
            else {
                ImGui::Text( "No selection made." );
            }
        }

        ImGui::End();
    }

} // namespace views
