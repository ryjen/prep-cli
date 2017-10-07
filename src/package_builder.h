#ifndef MICRANTHA_PREP_PACKAGE_BUILDER_H
#define MICRANTHA_PREP_PACKAGE_BUILDER_H

#include "environment.h"
#include "package_config.h"
#include "repository.h"

namespace micrantha
{
    namespace prep
    {
        /**
         * the executor of package building and repository actions
         */
        class PackageBuilder
        {
           public:
            //! constructor
            PackageBuilder();

            //! initializes this instance
            // @param opts the options to initialize with
            // @returns PREP_SUCESS or PREP_FAILURE if an error occured
            int initialize(const Options &opts);

            //! builds a package in a path
            // @param config the package config
            // @param opts the session options
            // @param path the path the source is in
            // @returns PREP_SUCCESS or PREP_FAILURE if an error occured
            int build(const Package &config, const Options &opts, const char *path);

            /**
             * removes a package from the repository
             * @config the package to remove
             * @opts the command line options
             * @return PREP_SUCCESS if removed, otherwise PREP_FAILURE
             */
            int remove(const Package &config, const Options &opts);

            /**
             * links a package to bin path
             * TODO: a specific version
             * @param config the package to link
             * @return PREP_SUCCESS if linked, otherwise PREP_FAILURE
             */
            int link_package(const Package &config) const;

            /**
             * unlinks a package from bin path
             * @param config the package to unlink
             * @return PREP_SUCCESS if unlinked, otherwise PREP_FAILURE
             */
            int unlink_package(const Package &config) const;

            /**
             * executes a package from bin path
             * @param config the package to execute
             * @param argc the number of arguments
             * @param argv the arguments to pass
             * @return PREP_SUCCESS if executed otherwise PREP_FAILURE
             */
            int execute(const Package &config, int argc, char *const *argv) const;

            /**
             * adds preps bin path to shell configuration
             * TODO: might deprecate this
             */
            void export_path() const;

            /**
             * gets the repository used for building
             */
            Repository *repository();

            /**
             * gets the directory of a package
             */
            std::string get_package_directory(const std::string &name) const;

           private:
            /**
             * internal method to remove a package directory structure
             * @param package_name the name of the package
             * @param opts the command line options
             */
            int remove(const std::string &package_name, const Options &opts);

            /**
             * internal method to build a package
             * @param p the package
             * @param path the path to build in
             * @return PREP_SUCCESS if built, otherwise PREP_FAILURE
             */
            int build_package(const Package &p, const Options &opts, const char *path);

            // fields
            Repository repo_;
        };
    }
}

#endif
