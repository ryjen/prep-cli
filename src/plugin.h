#ifndef RJ_PREP_PLUGIN_H
#define RJ_PREP_PLUGIN_H

#include <string>
#include <vector>
#include "package_config.h"

namespace rj
{
    namespace prep
    {
        class plugin
        {
           public:
            constexpr static const char *const MANIFEST_FILE = "manifest.json";
            plugin(const std::string &name);
            virtual ~plugin();
            plugin(const plugin &other) = delete;
            plugin(plugin &&other) = default;
            plugin &operator=(const plugin &other) = delete;
            plugin &operator=(plugin &&other) = default;
            bool is_valid() const;
            std::string name() const;
            std::string type() const;
            std::vector<std::string> return_values() const;
            std::string return_value() const;
            int on_load();
            int on_unload();
            int on_resolve(const package &config);
            int on_install(const package &config, const std::string &path);
            int on_remove(const package &config, const std::string &path);
            int on_build(const package &config, const std::string &sourcePath, const std::string &buildPath, const std::string &installPath);
            int load(const std::string &path);
            bool is_enabled() const;

           private:
            std::string plugin_name(const package &config) const;
            std::string plugin_location(const package &config) const;
            int execute(const std::string &method, const std::vector<std::string> &input = std::vector<std::string>());
            std::string name_;
            std::string executablePath_;
            std::string version_;
            std::string basePath_;
            std::string type_;
            std::vector<package_dependency> dependencies_;
            std::vector<std::string> returnValues_;
        };
    }
}

#endif
