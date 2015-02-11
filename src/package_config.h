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
        class package
        {
        public:
            virtual ~package();
            const char *name() const;
            const char *build_system() const;
            virtual int load(const std::string &path) = 0;
        protected:
            package();
            package(json_object *obj);
            json_object *values_;
        };

        class package_dependency : public package
        {
            friend class package_config;
        public:
            ~package_dependency();
            const char *location() const;
            int load(const std::string &path);
        protected:
            package_dependency(json_object *obj);
        };

        class package_config : public package
        {
        public:
            package_config();
            ~package_config();
            int load(const std::string &path);
            bool is_loaded() const;
            vector<package_dependency> dependencies() const;
        private:
            vector<package_dependency> dependencies_;
        };
    }
}

#endif
