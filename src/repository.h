#include "package_config.h"

namespace arg3
{
    namespace cpppm
    {
        class repository
        {
        public:
            void initialize();
            void set_global(bool value);
            int get_from_git(const std::string &url);
            int get(const package_config &config);
            int get_from_folder(const char *path);
        private:
            int build_autotools(const char *path);
            int build_cmake(const char *path);
            string get_path() const;
            const char *const get_home_dir() const;
            bool global_;

            static const char *const LOCAL_REPO;

            static const char *const GLOBAL_REPO;
        };
    }
}