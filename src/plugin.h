#ifndef MICRANTHA_PREP_PLUGIN_H
#define MICRANTHA_PREP_PLUGIN_H

#include <string>
#include <utility>
#include <vector>

namespace micrantha
{
    namespace prep
    {
        class Package;
        class PackageDependency;

        /**
         * compile a zipped copy of the plugins into binary.  This can
         * be extracted by prep on demand.
         * i would prefer an install script of some sort to do this,
         * but for now, it ensures the plugins are always available.
         */
        extern const unsigned char default_plugins_zip[];
        extern const unsigned int default_plugins_size;

        /**
         * represents a plugin
         */
        class Plugin
        {
           public:
            // types of hooks a plugin supports
            typedef enum { LOAD, UNLOAD, INSTALL, REMOVE, RESOLVE, BUILD } Hooks;

            /**
             * the plugin manifest file
             */
            constexpr static const char *const MANIFEST_FILE = "manifest.json";

            /**
             * a return value for executing a plugin.  has a code and a list of return values.
             * purposefully implicit constructors for flexible return values
             */
            typedef struct Result {
                int code;
                std::vector<std::string> values;

                Result(int c) : code(c)
                {
                }
                Result(int c, std::vector<std::string> r) : code(c), values(std::move(r))
                {
                }
                bool operator==(int value) const
                {
                    return code == value;
                }
            } Result;

            /* constructors*/
            explicit Plugin(const std::string &name);
            ~Plugin();

            /* non-copyable, non-movable */
            Plugin(const Plugin &other) = delete;
            Plugin(Plugin &&other) = default;
            Plugin &operator=(const Plugin &other) = delete;
            Plugin &operator=(Plugin &&other) = default;

            // validators
            bool is_valid() const;
            bool is_enabled() const;

            // properties
            std::string name() const;
            std::string type() const;
            std::string version() const;
            bool is_internal() const;
            Plugin &set_verbose(bool value);

            // callbacks for the different hooks
            Result on_load() const;
            Result on_unload() const;
            Result on_resolve(const std::string &location) const;
            Result on_resolve(const Package &config) const;
            Result on_install(const Package &config, const std::string &path) const;
            Result on_remove(const Package &config, const std::string &path) const;
            Result on_build(const Package &config, const std::string &sourcePath, const std::string &buildPath,
                            const std::string &installPath) const;

            /**
             * loads a plugin
             * @param path the path to the plugin folder containing a manifest
             * @return PREP_SUCCESS if successful, PREP_ERROR if not a valid plugin, otherwise PREP_FAILURE
             */
            int load(const std::string &path);

           private:
            /**
             * gets this plugins name.  can be overriden by config
             * @param config the package config
             * @return the configured name
             */
            std::string plugin_name(const Package &config) const;

            /**
             * gets this plugins location. can be overriden by config.
             * @param config the package config
             * @return the configured location
             */
            std::string plugin_location(const Package &config) const;

            /**
             * executes this plugin.  this is where the magic happens
             * @param method the type of hook being executed
             * @param input the list of input arguments
             * @return a result value.  The result code may contain PREP_SUCCESS if successful, PREP_ERROR if an
             * internal error occurred, or PREP_FAILURE if the plugin doesn't respond to the hook
             */
            Result execute(const Hooks &method,
                           const std::vector<std::string> &input = std::vector<std::string>()) const;

            // properties
            std::string name_;
            std::string executablePath_;
            std::string version_;
            std::string basePath_;
            std::string type_;
            bool verbose_;
        };
    }
}

#endif
