#ifndef INCLUDE_CPPPM_CONFIG
#define INCLUDE_CPPPM_CONFIG

#include "config.h"

#ifndef HAVE_JSON_C_H
#include <json/json.h>
#else
#include <json-c/json.h>
#endif
#include <string>
#include <vector>

using namespace std;

namespace arg3
{
    namespace prep
    {

        typedef struct options_s
        {
            options_s();
            bool global;
            string package_file;
            string location;
        } options;

        class package
        {
        public:
            virtual ~package();
            const char *name() const;
            const char *version() const;
            const char *build_system() const;
            virtual int load(const std::string &path, const options &opts) = 0;
            virtual const char *location() const;
        protected:
            package();
            package(json_object *obj);
            const char *get_str(const std::string &key) const;
            bool get_bool(const std::string &key) const;
            void set_str(const std::string &key, const std::string &value);
            void set_bool(const std::string &key, bool value);
            json_object *values_;
        };

        class package_dependency : public package
        {
            friend class package_config;
        public:
            ~package_dependency();
            int load(const std::string &path, const options &opts);
        protected:
            package_dependency(json_object *obj);
        };


        class package_config : public package
        {
        public:
            package_config();
            ~package_config();
            int load(const std::string &path, const options &opts);
            bool is_loaded() const;
            vector<package_dependency> dependencies() const;
        private:
            vector<package_dependency> dependencies_;
        };
    }
}

#endif
