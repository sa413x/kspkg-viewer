#pragma once

#include "content_processor.hpp"

namespace views {
    class image_content_processor : public content_processor {
    public:
        void process( const std::shared_ptr< kspkg::package >& package, const std::shared_ptr< kspkg::file >& file ) override;
    };
} // namespace views