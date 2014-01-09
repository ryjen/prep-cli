#ifndef INCLUDE_CPPPM_CONFIG
#define INCLUDE_CPPPM_CONFIG

#include <json/json.h>
#include <string>
#include <vector>

using namespace std;

namespace arg3
{
    namespace cpppm
    {
        class package_config
        {
        public:
            package_config();
            package_config(const std::string &path);
            ~package_config();
            bool load();
            void set_path(const std::string &path);
            string path() const;
            const char *name() const;
            const char *git_url() const;
            const char *build_system() const;
            bool is_loaded() const;
        private:
            json_object *values_;
            string path_;
        };
    }
}

#endif
