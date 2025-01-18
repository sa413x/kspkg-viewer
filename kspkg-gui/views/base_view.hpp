#pragma once

#include <vector>

namespace views {
    class base_view {
    public:
        base_view() {
            views_.push_back( this );
        }

        virtual ~base_view() = default;

        virtual void render() = 0;

        static void render_all() {
            for ( auto* view : views_ ) {
                view->render();
            }
        }

    private:
        static inline std::vector< base_view* > views_;
    };
} // namespace views