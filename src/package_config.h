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
            package();
            virtual ~package();
            const char *name() const;
            const char *build_system() const;
        protected:
            friend class package_config;
            package(json_object *obj);
            json_object *values_;
        };

        class package_config : public package
        {
        public:
            package_config();
            package_config(const std::string &path);
            ~package_config();
            int load();
            void set_path(const std::string &path);
            string path() const;
            bool is_loaded() const;
            bool is_temp_path() const;
            void set_temp_path(bool value);
            const vector<package> dependencies() const;
        private:
            string path_;
            bool isTemp_;
            vector<package> dependencies_;
        };
    }
}

#endif
