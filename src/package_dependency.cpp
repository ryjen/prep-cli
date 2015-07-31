
#include "package_config.h"
#include <fstream>
#include <sstream>

namespace arg3
{
    namespace prep
    {
        package_dependency::package_dependency(json_object *obj) : values_(obj)
        {
        }

        package_dependency::~package_dependency()
        {
            if (values_ != NULL)
            {
                json_object_put(values_);
                values_ = NULL;
            }
        }

        const char *package_dependency::name() const
        {
            json_object *obj;

            if (json_object_object_get_ex(values_, "name", &obj))
                return json_object_get_string(obj);

            return NULL;
        }

        const char *package_dependency::build_system() const
        {
            json_object *obj;

            if (json_object_object_get_ex(values_, "build_system", &obj))
                return json_object_get_string(obj);

            return NULL;
        }

        const char *package_dependency::git() const
        {
            json_object *obj;

            if (json_object_object_get_ex(values_, "git", &obj))
                return json_object_get_string(obj);

            return NULL;
        }

        std::string package_dependency::to_string() const
        {
            return json_object_to_json_string(values_);
        }
    }
}