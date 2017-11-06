
#include <sstream>

#include "environment.h"
#include "repository.h"
#include "util.h"
#include "common.h"

using namespace std;

namespace micrantha
{
    namespace prep
    {
        std::string environment::get(const std::string &key) {
            auto val = getenv(key.c_str());

            if (val == nullptr) {
                return "";
            }

            return val;
        }

        void environment::set(const std::string &key, const std::string &value, bool overwrite) {
            if (key.empty()) {
                return;
            }

            setenv(key.c_str(), value.c_str(), overwrite);
        }

        string environment::build_ldflags(const string &varName)
        {
            ostringstream buf;
            auto temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO) == PREP_SUCCESS) {
                buf << "-L" << Repository::GLOBAL_REPO << "/lib ";
            }

            if (directory_exists(temp) == PREP_SUCCESS) {
                buf << "-L" << temp << "/lib ";
            }

            temp = get(varName);

            if (!temp.empty()) {
                buf << temp;
            }

            return buf.str();
        }

        string environment::build_cflags(const string &varName)
        {
            ostringstream buf;
            auto temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO) == PREP_SUCCESS) {
                buf << "-I" << Repository::GLOBAL_REPO << "/include ";
            }

            if (directory_exists(temp) == PREP_SUCCESS) {
                buf << "-I" << temp << "/include ";
            }

            temp = get(varName);

            if (!temp.empty()) {
                buf << temp;
            }

            return buf.str();
        }

        string environment::build_path(const std::string &varName)
        {
            ostringstream buf;
            auto temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO) == PREP_SUCCESS) {
                buf << Repository::GLOBAL_REPO << "/bin:";
            }

            if (directory_exists(temp) == PREP_SUCCESS) {
                buf << temp << "/bin:";
            }

            temp = get(varName);

            if (!temp.empty()) {
                buf << temp;
            }

            return buf.str();
        }

        string environment::build_ldpath(const std::string &varName)
        {
            ostringstream buf;
            auto temp = Repository::get_local_repo();

            if (directory_exists(Repository::GLOBAL_REPO) == PREP_SUCCESS) {
                buf << Repository::GLOBAL_REPO << "/lib:";
            }

            if (directory_exists(temp) == PREP_SUCCESS) {
                buf << temp << "/lib:";
            }

            temp = get(varName);

            if (!temp.empty()) {
                buf << temp;
            }

            return buf.str();
        }

        std::vector<std::string> environment::build_env() {
            std::vector<std::string> env;

            for( const auto &entry : build_map()) {
                env.push_back(entry.first + "=" + entry.second);
            }
            return env;
        }

        std::map<std::string, std::string> environment::build_map()
        {
            return { {"CXXFLAGS", build_cflags("CXXFLAGS")},
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
