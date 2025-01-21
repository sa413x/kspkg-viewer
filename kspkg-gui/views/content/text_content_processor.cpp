#include "text_content_processor.hpp"

#include <imgui.h>

namespace views {
    void text_content_processor::process( const std::shared_ptr< kspkg::package >& package, const std::shared_ptr< kspkg::file >& file ) {
        const auto file_content = package->extract_file( file );
        if ( !file_content )
            return;

        ImGui::PushStyleColor( ImGuiCol_ChildBg, { 0.06f, 0.06f, 0.06f, 1.f } );
        ImGui::BeginChild( "FilePreview", ImVec2( 0, 0 ), 0, ImGuiWindowFlags_HorizontalScrollbar );

        const std::string file_content_str( file_content->begin(), file_content->end() );
        ImGui::TextUnformatted( file_content_str.c_str(), file_content_str.c_str() + file_content_str.size() );

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
} // namespace views