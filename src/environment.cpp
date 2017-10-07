
#include <sstream>

#include "environment.h"
#include "repository.h"
#include "util.h"

using namespace std;

namespace micrantha
{
    namespace prep
    {
        string environment::build_ldflags(const string &varName)
        {
            ostringstream buf;
            const char *temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO)) {
                buf << "-L" << Repository::GLOBAL_REPO << "/lib ";
            }

            if (directory_exists(temp)) {
                buf << "-L" << temp << "/lib ";
            }

            temp = getenv(varName.c_str());

            if (temp) {
                buf << temp;
            }

            return buf.str();
        }

        string environment::build_cflags(const string &varName)
        {
            ostringstream buf;
            const char *temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO)) {
                buf << "-I" << Repository::GLOBAL_REPO << "/include ";
            }

            if (directory_exists(temp)) {
                buf << "-I" << temp << "/include ";
            }

            temp = getenv(varName.c_str());

            if (temp) {
                buf << temp;
            }

            return buf.str();
        }

        string environment::build_path(const std::string &varName)
        {
            ostringstream buf;
            const char *temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO)) {
                buf << Repository::GLOBAL_REPO << "/bin:";
            }

            if (directory_exists(temp)) {
                buf << temp << "/bin:";
            }

            temp = getenv(varName.c_str());

            if (temp) {
                buf << temp;
            }

            return buf.str();
        }

        string environment::build_ldpath(const std::string &varName)
        {
            ostringstream buf;
            const char *temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO)) {
                buf << Repository::GLOBAL_REPO << "/lib:";
            }

            if (directory_exists(temp)) {
                buf << temp << "/lib:";
            }

            temp = getenv(varName.c_str());

            if (temp) {
                buf << temp;
            }

            return buf.str();
        }

        std::map<std::string, std::string> environment::build_cpp_variables()
        {
            return { {"CPPFLAGS", build_cflags("CPPFLAGS")},
                     {"LDFLAGS", build_ldflags("LDFLAGS")},
                     { "PATH", build_path("PATH") },
                     {
#ifdef __APPLE__
                            "DYLD_LIBRARY_PATH", build_ldpath("DYLD_LIBRARY_PATH")
#else
                            "LD_LIBRARY_PATH", build_ldpath("LD_LIBRARY_PATH")
#endif
                    }};
        }
    }
}
