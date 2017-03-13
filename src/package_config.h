#ifndef RJ_PREP_PACKAGE_CONFIG_H
#define RJ_PREP_PACKAGE_CONFIG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <string>
#include <vector>
#include "json.h"

using namespace std;

namespace rj
{
    namespace prep
    {
        typedef struct options_s {
            options_s();
            bool global;
            string package_file;
            string location;
            bool force_build;
        } options;

        class package_dependency;
        class plugin;

        class package
        {
           public:
            virtual ~package();
            std::string name() const;
            std::string version() const;
            vector<string> build_system() const;
            std::string build_options() const;
            std::string path() const;
            bool has_path() const;
            bool has_name() const;
            virtual int load(const std::string &path, const options &opts) = 0;
            virtual std::string location() const;
            void set_location(const std::string &value);
            bool is_loaded() const;
            vector<package_dependency> dependencies() const;
            std::string executable() const;
            void merge(const rj::json::object &other);
            rj::json::object get_plugin_config(const plugin *plugin) const;

           protected:
            package();
            package(const rj::json::object &obj);
            package(const package &other);
            package &operator=(const package &other);

            void init_dependencies();
            void init_build_system();

            rj::json::object values_;
            std::string path_;
            vector<package_dependency> dependencies_;
            vector<string> build_system_;
            friend class plugin;
        };

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
            package_dependency(const rj::json::object &obj);
        };


        class package_config : public package
        {
           public:
            package_config();
            package_config(const package_config &);
            ~package_config();
            package_config &operator=(const package_config &other);
            int load(const std::string &path, const options &opts);

           private:
            int resolve_package_file(const string &path, const string &filename, ifstream &file);
            int download_package_file(const string &path, const string &url, ifstream &file);
        };
    }
}

#endif
