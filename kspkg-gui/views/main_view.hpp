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
        void setup() override;
        void render() override;

    private:
        void on_install_russian_language() const;
        void on_remove_all_patches() const;

        static void build_hierarchy( const std::vector< std::shared_ptr< kspkg::file > >& files,
                                     const std::shared_ptr< file_node_t >& root );
        static bool has_matching_files( const std::shared_ptr< file_node_t >& node, const std::string& filter );
        static void render_hierarchy( const std::shared_ptr< file_node_t >& node, const std::string& name, const std::string& filter,
                                      std::shared_ptr< file_node_t >& selected_node );
        static bool has_extension( const std::string& file_name, const std::vector< std::string >& extensions );

        std::shared_ptr< file_node_t > root_node_;
        std::shared_ptr< kspkg::package > package_;
    };

    inline main_view main_view_instance;
} // namespace views