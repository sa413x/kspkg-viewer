#include "image_content_processor.hpp"

#include "render_extensions.hpp"

#include <d3d11.h>
#include <imgui.h>

namespace views {
    void image_content_processor::process( const std::shared_ptr< kspkg::package >& package, const std::shared_ptr< kspkg::file >& file ) {
        static ID3D11ShaderResourceView* image = nullptr;
        static std::string old_resource;
        static int width = 0, height = 0;

        if ( old_resource != file->get_name() ) {
            if ( image ) {
                image->Release();
                image = nullptr;
            }
            old_resource = file->get_name();
        }

        const auto file_content = package->extract_file( file );
        if ( !file_content )
            return;

        if ( !image ) {
            ImGui::RenderExtensions::LoadTextureFromMemory( file_content->data(), file_content->size(), &image, &width, &height );
        }

        if ( !image )
            return;

        ImGui::PushStyleColor( ImGuiCol_ChildBg, { 0.06f, 0.06f, 0.06f, 1.f } );
        ImGui::BeginChild( "FilePreview", ImVec2( 0, 0 ), 0, ImGuiWindowFlags_HorizontalScrollbar );

        ImGui::Image( reinterpret_cast< ImTextureID >( image ), { static_cast< float >( width ), static_cast< float >( height ) } );

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
} // namespace views