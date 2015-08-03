
#include "package_config.h"
#include <fstream>
#include <sstream>

namespace arg3
{
    namespace prep
    {
        package_dependency::package_dependency(json_object *obj) : package(obj)
        {

        }

        package_dependency::package_dependency(const package_dependency &other) : package(other)
        {}

        package_dependency::~package_dependency()
        {
            if (values_ != NULL)
            {
                json_object_put(values_);
                values_ = NULL;
            }
        }

        int package_dependency::load(const std::string &path, const options &opts)
        {
            return EXIT_SUCCESS;
        }

    }
}