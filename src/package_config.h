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
            ~package_config();
            bool load(const std::string &filename);
            string name() const;
            string include_dir() const;
            string source_dir() const;
            vector<string> link_options() const;
            vector<string> compile_options() const;
        private:
            vector<string> get_array(const std::string &name) const;
            json_object *values_;
        };
    }
}

#endif
