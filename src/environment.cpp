
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

            string flags = buf.str();

            if (flags.empty()) {
                return flags;
            }

            return varName + "=" + flags;
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

            string flags = buf.str();

            if (flags.empty()) {
                return flags;
            }

            return varName + "=" + flags;
        }

        string environment::build_path()
        {
            ostringstream buf;
            const char *temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO)) {
                buf << Repository::GLOBAL_REPO << "/bin:";
            }

            if (directory_exists(temp)) {
                buf << temp << "/bin:";
            }

            temp = getenv("PATH");

            if (temp) {
                buf << temp;
            }

            string flags = buf.str();

            if (flags.empty()) {
                return flags;
            }

            return "PATH=" + flags;
        }

        string environment::build_ldpath()
        {
            ostringstream buf;
            const char *temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO)) {
                buf << Repository::GLOBAL_REPO << "/lib:";
            }

            if (directory_exists(temp)) {
                buf << temp << "/lib:";
            }

#ifdef __APPLE__
            temp = getenv("DYLD_LIBRARY_PATH");
#else
            temp = getenv("LD_LIBRARY_PATH");
#endif

            if (temp) {
                buf << temp;
            }

            string flags = buf.str();

            if (flags.empty()) {
                return flags;
            }
#ifdef __APPLE__
            return "DYLD_LIBRARY_PATH=" + flags;
#else
            return "LD_LIBRARY_PATH=" + flags;
#endif
        }

        std::vector<std::string> environment::build_cpp_variables()
        {
            return {build_cflags("CPPFLAGS"), build_ldflags("LDFLAGS"), build_path(), build_ldpath()};
        }
    }
}
