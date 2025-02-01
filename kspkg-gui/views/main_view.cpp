#include <Windows.h>
#include <d3d11.h>

#include <imgui.h>
#include <ImGuiNotify.hpp>

#include <map>
#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <algorithm>

#include "main_view.hpp"
#include "content/content_processor_factory.hpp"

namespace views {

    void main_view::setup() {
        // const auto content_path = R"(D:\SteamLibrary\steamapps\common\Assetto Corsa EVO\content.kspkg)";
        const auto content_path = std::filesystem::current_path() / ".." / "content.kspkg";

        const auto package = kspkg::load_package( content_path );

        if ( !package ) {
            MessageBoxA( nullptr, package.error().c_str(), "Error", MB_ICONERROR );
            ExitProcess( 0 );
        }

        package_ = package.value();
        root_node_ = std::make_shared< file_node_t >();

        build_hierarchy( package_->get_files(), root_node_ );
    }

    void main_view::render() {
        static char filter_buffer[ 256 ] = "";
        static std::shared_ptr< file_node_t > selected_node = nullptr;

        const auto viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos( viewport->Pos );
        ImGui::SetNextWindowSize( viewport->Size );

        ImGui::Begin( "Main View", nullptr,
                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                          ImGuiWindowFlags_MenuBar );

        ImGui::BeginMenuBar();

        if ( ImGui::BeginMenu( "Addons" ) ) {
            if ( ImGui::MenuItem( "Install Russian Language", nullptr, nullptr, true ) ) {
                on_install_russian_language();
            }
            if ( ImGui::MenuItem( "Remove All Patches", nullptr, nullptr, true ) ) {
                on_remove_all_patches();
            }
            ImGui::EndMenu();
        }

        if ( ImGui::BeginMenu( "Help" ) ) {
            if ( ImGui::MenuItem( "GitHub Issues Page", nullptr, nullptr, true ) ) {
                ShellExecuteA( nullptr, "open", "https://github.com/sa413x/kspkg-viewer/issues", nullptr, nullptr, SW_SHOWNORMAL );
            }

            ImGui::Separator();
            ImGui::Text( "v0.4.0" );

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
                ImGui::Text( selected_node->is_file ? "File: %s" : "Directory: %s", selected_node->name.c_str() );

                ImGui::SameLine();

                if ( ImGui::Button( "Extract" ) ) {
                    const auto out_path = std::filesystem::current_path() / "__output__";

                    if ( selected_node->is_file ) {
                        if ( const auto file = package_->extract_file( selected_node->ref, out_path ); file ) {
                            ImGui::InsertNotification( ImGuiToast( ImGuiToastType::Info, 3000, "File extracted successfully to: %s",
                                                                   ( out_path / selected_node->name ).string().c_str() ) );
                        }
                        else {
                            ImGui::InsertNotification(
                                ImGuiToast( ImGuiToastType::Error, 3000, "Failed to extract file: %s", file.error().c_str() ) );
                        }
                    }
                    else {
                        size_t extracted = 0;
                        size_t errors = 0;

                        std::queue< std::shared_ptr< file_node_t > > queue;
                        queue.push( selected_node );

                        for ( ; !queue.empty(); queue.pop() ) {
                            const auto& node = queue.front();
                            if ( node->is_file ) {
                                if ( const auto file = package_->extract_file( node->ref, out_path ); file ) {
                                    extracted += 1;
                                }
                                else {
                                    ImGui::InsertNotification( ImGuiToast( ImGuiToastType::Error, 10000, "Failed to extract file %s: %s",
                                                                           node->ref->get_name().data(), file.error().c_str() ) );
                                    errors += 1;
                                }
                            }
                            else {
                                for ( const auto& [ _, node ] : node->children ) {
                                    queue.push( node );
                                }
                            }
                        }

                        ImGui::InsertNotification( ImGuiToast( ImGuiToastType::Info, 3000,
                                                               "Extracted %d file(s), failed to extract %d file(s)", extracted, errors ) );
                    }
                }

                if ( selected_node->is_file ) {
                    if ( const auto processor = content_processor_factory::create_processor( selected_node->name ) ) {
                        processor->process( package_, selected_node->ref );
                    }
                    else {
                        ImGui::Text( "Unsupported file type for preview." );
                    }
                }
            }
            else {
                ImGui::Text( "No selection made." );
            }
        }

        ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.f );

        ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.15f, 0.15f, 0.15f, 1.00f ) );

        ImGui::RenderNotifications();

        ImGui::PopStyleVar( 2 );
        ImGui::PopStyleColor( 1 );

        ImGui::End();
    }

    void main_view::on_install_russian_language() const {
        const auto lang_dir = std::filesystem::current_path() / "resources" / "ru_lang";
        const std::vector lang_files = {
            lang_dir / "cn.cars.loc",
            lang_dir / "cn.loc",
            lang_dir / "cn.tooltips.loc",
        };

        if ( const auto result = repack_package( package_, lang_files, R"(uiresources\localization\)" ) ) {
            ImGui::InsertNotification( ImGuiToast( ImGuiToastType::Info, 3000, "Russian language installed successfully." ) );
        }
        else {
            ImGui::InsertNotification(
                ImGuiToast( ImGuiToastType::Error, 3000, "Failed to install Russian language: %s", result.error().c_str() ) );
        }
    }

    void main_view::on_remove_all_patches() const {
        if ( const auto remove_result = remove_patches( package_ ) ) {
            ImGui::InsertNotification(
                ImGuiToast( ImGuiToastType::Info, 3000, *remove_result ? "All patches removed successfully." : "No patches found." ) );
        }
        else {
            ImGui::InsertNotification(
                ImGuiToast( ImGuiToastType::Error, 3000, "Failed to remove all patches: %s", remove_result.error().c_str() ) );
        }
    }

    void main_view::build_hierarchy( const std::vector< std::shared_ptr< kspkg::file > >& files,
                                     const std::shared_ptr< file_node_t >& root ) {
        for ( const auto& file : files ) {
            if ( file->is_directory() )
                continue;

            auto current_node = root;
            size_t start = 0, end;

            const std::string path = file->get_name().data();
            while ( ( end = path.find_first_of( "/\\", start ) ) != std::string::npos ) {
                if ( auto folder_name = path.substr( start, end - start ); !folder_name.empty() ) {
                    if ( !current_node->children.contains( folder_name ) ) {
                        current_node->children[ folder_name.data() ] = std::make_shared< file_node_t >();
                        current_node->children[ folder_name.data() ]->name = folder_name;
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

    bool main_view::has_matching_files( const std::shared_ptr< file_node_t >& node, const std::string& filter ) {
        if ( node->is_file ) {
            return filter.empty() || node->name.find( filter ) != std::string::npos;
        }

        return std::ranges::any_of( node->children,
                                    [ &filter ]( const auto& child ) { return has_matching_files( child.second, filter ); } );
    }

    void main_view::render_hierarchy( const std::shared_ptr< file_node_t >& node, const std::string& name, const std::string& filter,
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

            const bool opened = ImGui::TreeNodeEx( name.c_str(), selected_node == node ? ImGuiTreeNodeFlags_Selected : 0 );
            if ( ImGui::IsItemClicked() ) {
                selected_node = node;
            }
            if ( opened ) {
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

    bool main_view::has_extension( const std::string& file_name, const std::vector< std::string >& extensions ) {
        return std::ranges::any_of( extensions, [ &file_name ]( const auto& ext ) {
            return file_name.size() >= ext.size() && file_name.compare( file_name.size() - ext.size(), ext.size(), ext ) == 0;
        } );
    }

} // namespace views
