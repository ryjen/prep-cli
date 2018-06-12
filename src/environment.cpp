
#include <sstream>

#include "environment.h"
#include "repository.h"
#include "util.h"
#include "common.h"

namespace micrantha
{
    namespace prep
    {
        namespace build {
            namespace fs = filesystem;

            std::string flags(const std::string &varName, const std::string &flag, const std::string &folder)
            {
                std::ostringstream buf;
                auto temp = Repository::get_local_repo();

                if (fs::directory_exists(Repository::GLOBAL_REPO) == PREP_SUCCESS) {
                    buf << " " << flag << fs::build_path(Repository::GLOBAL_REPO, folder);
                }

                if (fs::directory_exists(temp) == PREP_SUCCESS) {
                    buf << " " << flag << fs::build_path(temp, folder);
                }

                temp = environment::get(varName);

                if (!temp.empty()) {
                    buf << " " << temp;
                }

                return buf.str();
            }

            std::string path(const std::string &varName, const std::string &folder)
            {
                std::vector<std::string> paths;

                auto temp = Repository::get_local_repo();

                if (fs::directory_exists(Repository::GLOBAL_REPO) == PREP_SUCCESS) {
                    paths.push_back(fs::build_path(Repository::GLOBAL_REPO, folder));
                }

                if (fs::directory_exists(temp) == PREP_SUCCESS) {
                    paths.push_back(fs::build_path(temp, folder));
                }

                temp = environment::get(varName);

                if (!temp.empty()) {
                    paths.push_back(temp);
                }


                std::ostringstream buf;

                for (auto it = paths.begin(); it != paths.end(); ++it) {
                    buf << *it;

                    if (it + 1 != paths.end()) {
                        buf << ":";
                    }
                }
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

        std::vector<std::string> environment::run_env() {

            std::vector<std::string> env;

            for( const auto &entry : run_map()) {
                env.push_back(entry.first + "=" + entry.second);
            }
            return env;
        }

        std::vector<std::string> environment::build_env() {
            std::vector<std::string> env;

            for( const auto &entry : build_map()) {
                env.push_back(entry.first + "=" + entry.second);
            }
            return env;
        }


        std::map<std::string, std::string> environment::run_map()
        {
            return {
                    { "LD_LIBRARY_PATH", build::path("LD_LIBRARY_PATH", "lib")},
                    { "PATH", build::path("PATH", "bin") },
                    { "TERM", environment::get("TERM") }
            };
        }

        std::map<std::string, std::string> environment::build_map()
        {
            auto map = run_map();

            map["CXXFLAGS"] = build::flags("CXXFLAGS", "-I", "include");
            map["LDFLAGS"] = build::flags("LDFLAGS", "-L", "lib");
            map["PKG_CONFIG_PATH"] = build::path("PKG_CONFIG_PATH", "lib/pkgconfig");

            return map;
        }
    }
}
