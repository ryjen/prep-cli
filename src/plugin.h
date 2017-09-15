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

        extern const unsigned char default_plugins_zip[];
        extern const unsigned int default_plugins_size;

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

            /**
             * a return value for executing a plugin.
             */
            typedef struct Result {
                int code;
                std::vector<std::string> values;
                Result(int c) : code(c)
                {
                }
                Result(int c, const std::vector<std::string> &r) : code(c), values(r)
                {
                }
                bool operator==(int value) const
                {
                    return code == value;
                }
            } Result;

            plugin();
            plugin(const std::string &name);
            ~plugin();
            plugin(const plugin &other) = delete;
            plugin(plugin &&other) = default;
            plugin &operator=(const plugin &other) = delete;
            plugin &operator=(plugin &&other) = default;

            // validators
            bool is_valid() const;
            bool is_enabled() const;

            // properties
            std::string name() const;
            std::string type() const;
            std::string version() const;
            bool is_internal() const;

            // callbacks
            Result on_load() const;
            Result on_unload() const;
            Result on_resolve(const std::string &location) const;
            Result on_resolve(const package &config) const;
            Result on_install(const package &config, const std::string &path) const;
            Result on_remove(const package &config, const std::string &path) const;
            Result on_build(const package &config, const std::string &sourcePath, const std::string &buildPath,
                            const std::string &installPath) const;

            /**
             * loads a plugin
             * @param path the path to the plugin folder containing a manifest
             * @return PREP_SUCCESS or PREP_FAILURE
             */
            int load(const std::string &path);

           private:
            std::string plugin_name(const package &config) const;
            std::string plugin_location(const package &config) const;
            Result execute(const std::string &method,
                           const std::vector<std::string> &input = std::vector<std::string>()) const;

            // properties
            std::string name_;
            std::string executablePath_;
            std::string version_;
            std::string basePath_;
            std::string type_;
            std::vector<package_dependency> dependencies_;
        };
    }
}

#endif
