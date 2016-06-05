#ifndef ARG3_PREP_PLUGIN_H
#define ARG3_PREP_PLUGIN_H

#include <vector>
#include <string>
#include <thread>
#include "package_config.h"

namespace arg3
{
    namespace prep
    {
        class plugin
        {
        public:
            plugin(const std::string &name);
            virtual ~plugin();
            plugin(const plugin &other) = delete;
            plugin(plugin &&other) = default;
            plugin &operator=(const plugin &other) = delete;
            plugin &operator=(plugin &&other) = default;
            bool is_valid() const;
            int on_load();
            int on_unload();
            int on_install(const package &config);
            int on_remove(const package &config);
            int load(const std::string &path);
            bool is_enabled() const;
        private:
            int execute(const std::string &method, const std::vector<std::string> &input = std::vector<std::string>());
            std::string name_;
            std::string executablePath_;
            std::string version_;
            std::string basePath_;
            std::vector<package_dependency> dependencies_;
            std::thread rw_thread_;
        };
    }
}

#endif
