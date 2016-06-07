#ifndef ARG3_PREP_PACKAGE_CONFIG_H
#define ARG3_PREP_PACKAGE_CONFIG_H

#include "config.h"

#ifndef HAVE_JSON_C_H
#include <json/json.h>
#else
#include <json-c/json.h>
#endif
#include <fstream>
#include <string>
#include <vector>

using namespace std;

namespace arg3
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
            bool is_loaded() const;
            vector<package_dependency> dependencies() const;
            std::string executable() const;

           protected:
            package();
            package(json_object *obj);
            package(const package &other);
            package &operator=(const package &other);
            std::string get_str(const std::string &key) const;
            bool get_bool(const std::string &key) const;
            void set_str(const std::string &key, const std::string &value);
            void set_bool(const std::string &key, bool value);
            std::string get_plugin_name(plugin *plugin) const;
            json_object *values_;
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
            package_dependency &operator=(const package_dependency &);
            int load(const std::string &path, const options &opts);

           protected:
            package_dependency(json_object *obj);
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
