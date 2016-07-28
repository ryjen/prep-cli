#ifndef RJ_PREP_PACKAGE_BUILDER_H
#define RJ_PREP_PACKAGE_BUILDER_H

#include "environment.h"
#include "package_config.h"
#include "repository.h"

namespace rj
{
    namespace prep
    {
        class package_builder
        {
           public:
            //! initializes this instance
            // @param opts the options to initialize with
            // @returns PREP_SUCESS or PREP_FAILURE if an error occured
            int initialize(const options &opts);

            //! builds a package in a path
            // @param config the package config
            // @param opts the session options
            // @param path the path the source is in
            // @returns PREP_SUCCESS or PREP_FAILURE if an error occured
            int build(const package &config, options &opts, const char* path);

            int build_from_folder(options &opts, const char *path);
            string repo_path() const;
            int remove(package &config, options &opts, const char *package);
            int remove(const std::string &package_name, options &opts);
            int link_package(const package &config) const;
            int unlink_package(const package &config) const;
            int execute(const package &config, int argc, char * const *argv) const;

           private:
            int build_package(const package &p, const char *path);
            int build_commands(const package &config, const char *path, const vector<string> &commands);

            string get_install_dir(const package &config) const;

            repository repo_;

            bool force_build_;
        };
    }
}

#endif
