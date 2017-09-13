
#include "common.h"
#include "package_config.h"

namespace micrantha
{
    namespace prep
    {
        package_dependency::package_dependency(const package::json_type &obj) : package(obj)
        {
        }

        package_dependency::package_dependency(const package_dependency &other) : package(other)
        {
        }

        package_dependency::~package_dependency()
        {
        }

        int package_dependency::load(const std::string &path, const options &opts)
        {
            return PREP_SUCCESS;
        }
    }
}
