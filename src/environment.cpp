
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
        namespace build {
            string flags(const string &varName, const string &flag, const string &folder)
            {
                ostringstream buf;
                auto temp = Repository::get_local_repo();

                if (directory_exists(Repository::GLOBAL_REPO) == PREP_SUCCESS) {
                    buf << flag << build_sys_path(Repository::GLOBAL_REPO, folder);
                }

                if (directory_exists(temp) == PREP_SUCCESS) {
                    buf << flag << build_sys_path(temp, folder);
                }

                temp = environment::get(varName);

                if (!temp.empty()) {
                    buf << temp;
                }

                return buf.str();
            }

            string path(const std::string &varName, const std::string &folder)
            {
                std::vector<std::string> paths;

                auto temp = Repository::get_local_repo();

                if (directory_exists(Repository::GLOBAL_REPO) == PREP_SUCCESS) {
                    paths.push_back(build_sys_path(Repository::GLOBAL_REPO, folder));
                }

                if (directory_exists(temp) == PREP_SUCCESS) {
                    paths.push_back(build_sys_path(temp, folder));
                }

                temp = environment::get(varName);

                if (!temp.empty()) {
                    paths.push_back(temp);
                }

                ostringstream buf;
                std::copy(paths.begin(), paths.end(), std::ostream_iterator<std::string>(buf, ":"));
                return buf.str();
            }
        }

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

        std::vector<std::string> environment::build_env() {
            std::vector<std::string> env;

            for( const auto &entry : build_map()) {
                env.push_back(entry.first + "=" + entry.second);
            }
            return env;
        }

        std::map<std::string, std::string> environment::build_map()
        {
            return { { "CXXFLAGS", build::flags("CXXFLAGS", "-I", "include") },
                     { "LDFLAGS", build::flags("LDFLAGS", "-L", "lib") },
                     { "PATH", build::path("PATH", "bin") },
                     { "PKG_CONFIG_PATH", build::path("PKG_CONFIG_PATH", "lib/pkgconfig") },
                     {
#ifdef __APPLE__
                            "DYLD_LIBRARY_PATH", build::path("DYLD_LIBRARY_PATH", "lib")
#else
                            "LD_LIBRARY_PATH", build::path("LD_LIBRARY_PATH", "lib")
#endif
                    }};
        }
    }
}
