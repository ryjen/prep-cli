
#include "package_config.h"
#include <fstream>
#include <sstream>

namespace arg3
{
    namespace prep
    {
        package::package() : values_(NULL)
        {

        }

        package::package(json_object *obj) : values_(obj)
        {}

        package_config::package_config() : isTemp_(false)
        {

        }
        package_config::package_config(const std::string &path) : package(), path_(path), isTemp_(false)
        {

        }

        package::~package()
        {
            if (values_ != NULL)
            {
                json_object_put(values_);
                values_ = NULL;
            }
        }

        package_config::~package_config()
        {
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

        bool package_config::is_temp_path() const
        {
            return isTemp_;
        }

        void package_config::set_temp_path(bool value)
        {
            isTemp_ = value;
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

            file.open(path_ + "/package.json");

            if (!file.is_open())
            {
                printf("unabel to open %s/package.json\n", path_.c_str());
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

            json_object *depjson;

            if (json_object_object_get_ex(values_, "dependencies", &depjson))
            {
                int len = json_object_array_length(depjson);

                for (int i = 0; i < len; i++)
                {

                    json_object *dep = json_object_array_get_idx(depjson, i);

                    dependencies_.push_back(package(json_object_get(dep)));
                }
            }

            return EXIT_SUCCESS;
        }

        const char *package::name() const
        {
            json_object *obj;

            if (json_object_object_get_ex(values_, "name", &obj))
                return json_object_get_string(obj);

            return NULL;
        }

        const char *package::build_system() const
        {
            json_object *obj;

            if (json_object_object_get_ex(values_, "build_system", &obj))
                return json_object_get_string(obj);

            return NULL;
        }

    }
}