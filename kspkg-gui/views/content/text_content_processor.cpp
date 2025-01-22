#include "text_content_processor.hpp"

#include <imgui.h>
#include "text_editor/text_editor.hpp"

namespace views {
    void text_content_processor::process( const std::shared_ptr< kspkg::package >& package, const std::shared_ptr< kspkg::file >& file ) {
        static std::map< std::string, TextEditor::LanguageDefinition > ext_map = {
            { ".html", TextEditor::LanguageDefinition::HTML() },
            { ".loc", TextEditor::LanguageDefinition::HTML() },
        };

        static std::shared_ptr< TextEditor > editor;
        if ( !editor ) {
            editor = std::make_shared< TextEditor >();
            editor->SetReadOnly( true );
        }

        static std::string old_resource;

        if ( old_resource != file->get_name() ) {
            if ( const auto file_content = package->extract_file( file ) ) {
                const std::string file_content_str( file_content->begin(), file_content->end() );
                editor->SetText( file_content_str );
                editor->SetCursorPosition( {} );

                if ( const auto ext = std::filesystem::path( file->get_name() ).extension().string(); ext_map.contains( ext ) ) {
                    editor->SetColorizerEnable( true );
                    editor->SetLanguageDefinition( ext_map[ ext ] );
                }
                else {
                    editor->SetColorizerEnable( false );
                }

                old_resource = file->get_name();
            }
        }

        editor->Render( "TextEditor", {}, true );
    }
} // namespace views