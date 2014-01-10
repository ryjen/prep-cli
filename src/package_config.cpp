
#include "package_config.h"
#include <fstream>
#include <sstream>

namespace arg3
{
    namespace cpppm
    {
        package_config::package_config() : values_(NULL)
        {

        }

        package_config::package_config(const std::string &path) : values_(NULL), path_(path)
        {

        }

        package_config::~package_config()
        {
            if (values_ != NULL)
            {
                json_object_put(values_);
                values_ = NULL;
            }
        }

        string package_config::path() const
        {
            return path_;
        }

        void package_config::set_path(const std::string &path)
        {
            path_ = path;
        }

        bool package_config::is_loaded() const
        {
            return values_ != NULL;
        }

        int package_config::load()
        {
            ifstream file;

            if (path_.empty())
                return EXIT_FAILURE;

            if (values_ != NULL)
            {
                json_object_put(values_);
                values_ = NULL;
            }

            file.open(path_ + "/cpppm.conf");

            if (!file.is_open())
            {
                printf("unabel to open %s/cpppm.conf\n", path_.c_str());
                return EXIT_FAILURE;
            }

            std::ostringstream os;

            file >> os.rdbuf();

            values_ = json_tokener_parse(os.str().c_str());

            if (values_ == NULL)
            {
                printf("invalid configuration for %s\n", os.str().c_str());
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }

        const char *package_config::name() const
        {
            json_object *obj = json_object_object_get(values_, "name");
            return json_object_get_string(obj);
        }

        const char *package_config::build_system() const
        {
            json_object *obj = json_object_object_get(values_, "build_system");
            return json_object_get_string(obj);
        }

    }
}