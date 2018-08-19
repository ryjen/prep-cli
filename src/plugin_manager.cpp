#include "plugin_manager.h"
#include "common.h"
#include "options.h"
#include "log.h"


namespace micrantha {
    namespace prep {

        int PluginManager::execute(const Options &opts, int argc, char *const *argv) const {

            int index = 0;
            const char *command = nullptr;

            if (argc == 0) {
                command = "list";
            } else {
                command = argv[index++];
            }

            if (string::equals(command, "list")) {
                return list(opts);
            }

            if (string::equals(command, "install")) {

                if (index >= argc) {
                    log::error("install which plugin?");
                    return PREP_FAILURE;
                }
                return install(argv[index]);
            }

            if (string::equals(command, "remove")) {
                if (index >= argc) {
                    log::error("remove which plugin?");
                    return PREP_FAILURE;
                }

                return remove(argv[index]);
            }

            if (string::equals(command, "enable")) {
                if (index >= argc) {
                    log::error("enable which plugin?");
                    return PREP_FAILURE;
                }

                return enable(argv[index]);
            }

            if (string::equals(command, "disable")) {
                if (index >= argc) {
                    log::error("disable which plugin?");
                    return PREP_FAILURE;
                }

                return disable(argv[index]);
            }

            if (string::equals(command, "update")) {
              return update(index >= argc ? nullptr : argv[index]);
            }

            if (!string::equals(command, "help")) {
                log::error("Unknown command '", command, "'");
            }
            help(opts);
            return PREP_FAILURE;
        }

        void PluginManager::help(const Options &opts) const {
            io::println(color::g("Syntax"), ": ", opts.exe, " plugins help");

            io::println(std::setw(12), opts.exe, " plugins list");
            io::println(std::setw(12), opts.exe, " plugins install <file/url>");
            io::println(std::setw(12), opts.exe, " plugins remove <name>");
            io::println(std::setw(12), opts.exe, " plugins enable <name>");
            io::println(std::setw(12), opts.exe, " plugins disable <name>");
            io::println(std::setw(12), opts.exe, " plugins update [name]");
        }

        int PluginManager::install(const std::string &path) const {
            log::error("Not implemented.");
            return PREP_SUCCESS;
        }

        int PluginManager::remove(const std::string &name) const {
             auto plugin = repo_.get_plugin_by_name(name);

            if (plugin == nullptr) {
                log::error("Unknown plugin '", name, "'");
                return PREP_FAILURE;
            }

            if (!filesystem::directory_exists(plugin->basePath_)) {
                log::error("Plugin is already uninstalled");
                return PREP_FAILURE;
            }

            auto res = filesystem::remove_directory(plugin->basePath_);

            if (res != PREP_SUCCESS) {
                log::error("Unable to remove plugin");
                return PREP_FAILURE;
            }

            repo_.validPlugins_.remove(plugin);
            log::info("Plugin '", name, "' removed.");
            return PREP_SUCCESS;
        }

        int PluginManager::enable(const std::string &name) const {

            auto plugin = repo_.get_plugin_by_name(name);

            if (plugin == nullptr) {
                log::error("Unknown plugin '", name, "'");
                return PREP_FAILURE;
            }

            auto res = plugin->set_enabled(true).save();

            if (res != PREP_SUCCESS) {
                log::error("Unable to enable plugin '", name, "'");
            } else {
                log::info("Enabled plugin '", name, "'");
            }
            return res;
        }

        int PluginManager::disable(const std::string &name) const {
            auto plugin = repo_.get_plugin_by_name(name);

            if (plugin == nullptr) {
                log::error("Unknown plugin '", name, "'");
                return PREP_FAILURE;
            }

            auto res = plugin->set_enabled(false).save();

            if (res != PREP_SUCCESS) {
                log::error("Unable to disable plugin '", name, "'");
            } else {
                log::info("Disabled plugin '", name, "'");
            }
            return res;
        }

        int PluginManager::update(const std::string &name) const {
          log::error("not implemented");
          return PREP_FAILURE;
        }

        int PluginManager::list(const Options &opts) const {

            auto fill = io::println(color::green, std::left, std::setw(15), "Name", color::yellow, std::setw(7), "Enabled", color::clear).fill();
            io::println(color::white, std::setfill('-'), std::setw(22), "-", std::setfill(fill));

            for (auto &plugin : repo_.validPlugins_) {
                io::println(std::left, std::setw(15), plugin->name(), std::setw(7), (plugin->is_enabled() ? "Yes" : "No"));
            }
            return PREP_SUCCESS;
        }

    }
}
