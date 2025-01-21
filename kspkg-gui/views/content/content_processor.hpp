#pragma once

#include <kspkg-core/core.hpp>

#include <memory>

namespace views {
    class content_processor {
    public:
        virtual ~content_processor() = default;
        virtual void process( const std::shared_ptr< kspkg::package >& package, const std::shared_ptr< kspkg::file >& file ) = 0;
    };
} // namespace views
