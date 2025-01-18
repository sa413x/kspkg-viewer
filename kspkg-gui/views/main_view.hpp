#pragma once

#include "base_view.hpp"

#include <map>
#include <memory>
#include <string>

namespace kspkg {
    class package;
    class file;
} // namespace kspkg

namespace views {
    struct file_node_t {
        std::map< std::string, std::shared_ptr< file_node_t > > children;
        bool is_file = false;
        std::string name;
        std::shared_ptr< kspkg::file > ref;
    };

    class main_view final : public base_view {
    public:
        void render() override;

    private:
        std::shared_ptr< file_node_t > root_node_;
        std::shared_ptr< kspkg::package > package_;
    };

    inline main_view main_view_instance;
} // namespace views