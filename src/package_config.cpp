
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

        package_dependency::package_dependency(json_object *obj) : package(obj)
        {}

        package_config::package_config()
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

        package_dependency::~package_dependency()
        {}

        bool package_config::is_loaded() const
        {
            return values_ != NULL;
        }

        vector<package_dependency> package_config::dependencies() const
        {
            return dependencies_;
        }
        int package_config::load(const std::string &path)
        {
            ifstream file;
            json_object *depjson;
            std::ostringstream os;

            if (path.empty()) {
                return EXIT_FAILURE;
            }

            if (values_ != NULL)
            {
                json_object_put(values_);
                values_ = NULL;
            }

            file.open(path + "/package.json");

            if (!file.is_open())
            {
                printf("unabel to open %s/package.json\n", path.c_str());
                return EXIT_FAILURE;
            }

            file >> os.rdbuf();

            values_ = json_tokener_parse(os.str().c_str());

            if (values_ == NULL)
            {
                printf("invalid configuration for %s\n", os.str().c_str());
                return EXIT_FAILURE;
            }

            if (json_object_object_get_ex(values_, "dependencies", &depjson))
            {
                int len = json_object_array_length(depjson);

                for (int i = 0; i < len; i++)
                {
                    json_object *dep = json_object_array_get_idx(depjson, i);

                    dependencies_.push_back(package_dependency(json_object_get(dep)));
                }
            }

            return EXIT_SUCCESS;
        }

        int package_dependency::load(const std::string &path)
        {
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

        const char *package_dependency::location() const
        {
            json_object *obj;

            if (json_object_object_get_ex(values_, "location", &obj))
                return json_object_get_string(obj);

            return NULL;
        }

    }
}