#ifndef MICRANTHA_PREP_PACKAGE_CONFIG_H
#define MICRANTHA_PREP_PACKAGE_CONFIG_H

#include "json.hpp"
#include <fstream>
#include <string>
#include <vector>

namespace micrantha
{
    namespace prep
    {
        typedef struct options_s {
            options_s();
            bool global;
            std::string package_file;
            std::string location;
            bool force_build;
            std::string executable;
        } options;

        class package_dependency;
        class plugin;

        /**
         * a package
         */
        class package
        {
          public:
            typedef nlohmann::json json_type;
            virtual ~package();

            /* properties */
            std::string name() const;
            std::string version() const;
            std::vector<std::string> build_system() const;
            std::string build_options() const;
            std::string path() const;
            virtual std::string location() const;
            std::vector<package_dependency> dependencies() const;
            std::string executable() const;

            /* validators */
            bool has_path() const;
            bool has_name() const;
            bool is_loaded() const;

            virtual int load(const std::string &path, const options &opts) = 0;
            void set_location(const std::string &value);
            void merge(const json_type &other);
            json_type get_plugin_config(const plugin *plugin) const;


            int dependency_count(const std::string &package_name) const;

          protected:
            package();
            package(const json_type &obj);
            package(const package &other);
            package &operator=(const package &other);

            void init_dependencies();
            void init_build_system();

            json_type values_;
            std::string path_;
            std::vector<package_dependency> dependencies_;
            std::vector<std::string> build_system_;
            friend class plugin;
        };

        /**
         * configuration for a package dependency
         */
        class package_dependency : public package
        {
            friend class package;
            friend class package_config;

          public:
            ~package_dependency();
            package_dependency(const package_dependency &other);
            package_dependency &operator=(const package_dependency &) = default;
            int load(const std::string &path, const options &opts);

          protected:
            package_dependency(const json_type &obj);
        };


        /**
         * configuration for a package
         */
        class package_config : public package
        {
          public:
            package_config();
            package_config(const package_config &);
            ~package_config();
            package_config &operator=(const package_config &other);
            int load(const std::string &path, const options &opts);

          private:
            int resolve_package_file(const std::string &path, const std::string &filename, std::ifstream &file);
            int download_package_file(const std::string &path, const std::string &url, std::ifstream &file);
        };
    }
}

#endif
