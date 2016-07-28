#ifndef RJ_PREP_ENVIRONMENT_H
#define RJ_PREP_ENVIRONMENT_H

#include <string>
#include <vector>

namespace rj
{
    namespace prep
    {
        namespace environment
        {
            std::string build_cflags(const std::string &varName);
            std::string build_ldflags(const std::string &varName);
            std::string build_path();
            std::string build_ldpath();

            std::vector<std::string> build_cpp_variables();

            int execute(const char *command, const char *path);
        }
    }
}

#endif
