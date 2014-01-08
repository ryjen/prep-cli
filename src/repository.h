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
            void get(const std::string &url);
        private:
            string get_path() const;
            bool global_;
        };
    }
}