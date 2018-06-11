#include "plugin_manager.h"
#include "common.h"


namespace micrantha {
    namespace prep {

        int PluginManager::execute(const Options &opts, int argc, char *const *argv) const {


            help(opts);
            return PREP_FAILURE;
        }

        void PluginManager::help(const Options &opts) const {
            printf("Syntax: %s plugins help\n", opts.exe);
            printf("        %s plugins install [file,url]\n", opts.exe);
            printf("        %s plugins remove [name]\n", opts.exe);
            printf("        %s plugins enable [name]\n", opts.exe);
            printf("        %s plugins disable [name]\n", opts.exe);
        }

        int PluginManager::install(const std::string &path) const {
            return PREP_SUCCESS;
        }

        int PluginManager::remove(const std::string &name) const {
            return PREP_SUCCESS;
        }

        int PluginManager::enable(const std::string &name) const {
            return PREP_SUCCESS;
        }

        int PluginManager::disable(const std::string &name) const {
            return PREP_SUCCESS;
        }

    }
}
