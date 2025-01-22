#pragma once

#include "image_content_processor.hpp"
#include "text_content_processor.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <memory>

namespace views {
    class content_processor_factory {
    public:
        static std::shared_ptr< content_processor > create_processor( const std::string& file_name ) {
            static const std::vector< std::string > kTextExtensions = { ".txt", ".ini", ".json", ".html", ".css", ".js",
                                                                        ".ts",  ".loc", ".data", ".md",   ".lut", ".csv" };
            static const std::vector< std::string > kImageExtensions = { ".png", ".jpg", ".svg" };

            if ( has_extension( file_name, kTextExtensions ) ) {
                return std::make_shared< text_content_processor >();
            }
            if ( has_extension( file_name, kImageExtensions ) ) {
                return std::make_shared< image_content_processor >();
            }

            return nullptr;
        }

    private:
        static bool has_extension( const std::string& file_name, const std::vector< std::string >& extensions ) {
            return std::ranges::any_of( extensions, [ &file_name ]( const auto& ext ) {
                return file_name.size() >= ext.size() && file_name.compare( file_name.size() - ext.size(), ext.size(), ext ) == 0;
            } );
        }
    };

} // namespace views