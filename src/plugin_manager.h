#ifndef MICRANTHA_PREP_PLUGIN_MANAGER_H
#define MICRANTHA_PREP_PLUGIN_MANAGER_H

#include "repository.h"

namespace micrantha {
    namespace prep {

        class PluginManager {
        public:
            PluginManager(Repository &repo) : repo_(repo) {}

            int execute(const Options &opts, int argc, char *const argv[]) const;
        private:

            void help(const Options &opts) const;

            int install(const std::string &path) const;

            int remove(const std::string &name) const;

            int enable(const std::string &name) const;

            int disable(const std::string &name) const;

            int list(const Options &opts) const;

            // fields
            Repository &repo_;
        };

    }
}

#endif

