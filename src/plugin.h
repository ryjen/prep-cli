#ifndef MICRANTHA_PREP_PLUGIN_H
#define MICRANTHA_PREP_PLUGIN_H

#include <string>
#include <vector>

namespace micrantha
{
    namespace prep
    {
        class package;
        class package_dependency;

        /**
         * represents a plugin
         */
        class plugin
        {
          public:
            /**
             * the plugin manifest file
             */
            constexpr static const char *const MANIFEST_FILE = "manifest.json";

            plugin(const std::string &name);
            ~plugin();
            plugin(const plugin &other) = delete;
            plugin(plugin &&other)      = default;
            plugin &operator=(const plugin &other) = delete;
            plugin &operator=(plugin &&other) = default;

            // validators
            bool is_valid() const;
            bool is_enabled() const;

            // properties
            std::string name() const;
            std::string type() const;
            std::vector<std::string> return_values() const;
            std::string return_value() const;

            // callbacks
            int on_load();
            int on_unload();
            int on_resolve(const std::string &location);
            int on_resolve(const package &config);
            int on_install(const package &config, const std::string &path);
            int on_remove(const package &config, const std::string &path);
            int on_build(const package &config, const std::string &sourcePath, const std::string &buildPath,
                         const std::string &installPath);

            /**
             * loads a plugin
             * @param path the path to the plugin folder containing a manifest
             * @return PREP_SUCCESS or PREP_FAILURE
             */
            int load(const std::string &path);

          private:
            std::string plugin_name(const package &config) const;
            std::string plugin_location(const package &config) const;
            int execute(const std::string &method, const std::vector<std::string> &input = std::vector<std::string>());

            // properties
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
