#include "package_builder.h"
#include "package_resolver.h"
#include "util.h"
#include "log.h"
#include "common.h"


namespace arg3
{
    namespace prep
    {

        int package_builder::initialize(const options &opts)
        {
            char path[BUFSIZ + 1] = {0};

            if (repo_.initialize(opts)) {
                return PREP_FAILURE;
            }

            force_build_ = opts.force_build;

            return repo_.validate();
        }

        int package_builder::build_package(const package & config, const char *path)
        {
            string installPath;

            if (!config.is_loaded()) {
                log_error("config not loaded");
                return PREP_FAILURE;
            }

            if (!force_build_ && repo_.has_meta(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
            }

            installPath = repo_.get_install_path(config.name());

            log_trace("Installing to %s...", installPath.c_str());

            if (!directory_exists(installPath.c_str())) {
                log_trace("Creating %s...", installPath.c_str());
                if (mkpath(installPath.c_str(), 0777)) {
                    log_errno(errno);
                    return PREP_FAILURE;
                }
            }

            if (!str_cmp(config.build_system(), "autotools"))
            {
                if ( build_autotools(config, path, installPath.c_str()) ) {
                    return PREP_FAILURE;
                }
            }
            else if (!str_cmp(config.build_system(), "cmake"))
            {
                if ( build_cmake(config, path, installPath.c_str()) ) {
                    return PREP_FAILURE;
                }
            }
            else
            {
                log_error("unknown build system '%s'", config.build_system());
                return PREP_FAILURE;
            }

            if (repo_.save_meta(config)) {
                log_error("unable to save meta data");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int package_builder::remove(const package & config, options & opts)
        {
            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            return remove(config.name(), opts);
        }

        int package_builder::remove(const string &package_name, options &opts)
        {
            string installDir = repo_.get_install_path(package_name);

            if (!directory_exists(installDir.c_str())) {
                log_info("%s is not installed", package_name.c_str());
                return PREP_SUCCESS;
            }

            int depCount = repo_.dependency_count(package_name, opts);

            if (depCount > 0) {
                log_warn("%d packages depend on %s", depCount, package_name.c_str());
                return PREP_FAILURE;
            }

            if (repo_.unlink_directory(installDir.c_str()))
            {
                log_error("unable to unlink package %s", package_name.c_str());
                return PREP_FAILURE;
            }

            if (remove_directory(installDir.c_str())) {
                log_error("unable to remove package %s", installDir.c_str());
                return PREP_FAILURE;
            }

            installDir = repo_.get_meta_path(package_name);

            if (remove_directory(installDir.c_str())) {
                log_error("unable to remove meta package %s", installDir.c_str());
                return PREP_FAILURE;
            }

            log_info("%s package removed.", package_name.c_str());

            return PREP_SUCCESS;
        }

        int package_builder::build(const package & config, options & opts, const std::string & path)
        {
            string installDir;

            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            log_trace("Building from [%s]...", path.c_str());

            if (config.location() != NULL) {
                repo_.save_history(config.location(), path);
            }

            installDir = repo_.get_install_path(config.name());

            if (!directory_exists(installDir.c_str())) {
                if (mkpath(installDir.c_str(), 0777)) {
                    log_error("could not create [%s] (%s)", installDir.c_str(), strerror(errno));
                    return PREP_FAILURE;
                }
            }

            for (package_dependency &p : config.dependencies())
            {
                package_resolver resolver;
                string working_dir;

                log_info("Building [%s] dependency [%s]...", config.name(), p.name());

                working_dir = repo_.exists_in_history(p.location());

                if (working_dir.empty() || !directory_exists(working_dir.c_str())) {

                    if (str_empty(p.location())) {
                        log_error("[%s] dependency [%s] has no location", config.name(), p.name());
                        continue;
                    }

                    if (resolver.resolve_package(p, opts, p.location())) {
                        return PREP_FAILURE;
                    }

                    working_dir = resolver.working_dir();
                }

                if (build(p, opts, working_dir)) {
                    log_error("unable to build dependency %s", p.name());
                    remove_directory(working_dir.c_str());
                    return PREP_FAILURE;
                }
            }

            if (repo_.has_meta(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
            }

            if ( build_package(config, path.c_str()) ) {
		              return PREP_FAILURE;
            }

            if ( repo_.link_directory(installDir.c_str()) ) {
                log_error("Unable to link package");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int package_builder::link_package(const package & config) const
        {
            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            string packageInstall = repo_.get_install_path(config.name());

            return repo_.link_directory(packageInstall.c_str());
        }

        int package_builder::unlink_package(const package & config) const
        {
            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            string packageInstall = repo_.get_install_path(config.name());

            return repo_.unlink_directory(packageInstall.c_str());
        }

        int package_builder::build_cmake(const package & config, const char *path, const char *toPath)
        {
            char buf[BUFSIZ + 1] = {0};
            const char *buildopts = NULL;

            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            log_debug("building cmake [%s]", path);

            buildopts = config.build_options();

            if (buildopts == NULL) {
                buildopts = "";
            }

            snprintf(buf, BUFSIZ, "cmake -DCMAKE_INSTALL_PREFIX:PATH=%s %s .", toPath, buildopts);

            if (env_.execute(buf, path))
            {
                log_error("unable to execute cmake");
                return PREP_FAILURE;
            }

            return build_make(config, path);
        }

        int package_builder::build_make(const package & config, const char *path)
        {
            if (env_.execute("make -j2 install", path))
            {
                log_error("unable to execute make");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int package_builder::build_autotools(const package & config, const char *path, const char *toPath)
        {
            char buf[BUFSIZ + 1] = {0};
            const char *buildopts = NULL;

            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            log_debug("building autotools [%s]", path);

            buildopts = config.build_options();

            if (buildopts == NULL) {
                buildopts = "";
            }

            snprintf(buf, BUFSIZ, "%s/configure", path);

            if (file_exists(buf)) {
                snprintf(buf, BUFSIZ, "./configure --prefix=%s %s", toPath, buildopts);
            } else {

                snprintf(buf, BUFSIZ, "%s/autogen.sh", path);

                if (!file_exists(buf)) {
                    log_error("Don't know how to build %s... no autotools configuration found.", config.name());
                    return PREP_FAILURE;
                }

                snprintf(buf, BUFSIZ, "./autogen.sh --prefix=%s %s", toPath, buildopts);
            }

            if (env_.execute(buf, path))
            {
                log_error("unable to execute configure");
                return PREP_FAILURE;
            }

            return build_make(config, path);
        }

        int package_builder::build_from_folder(options & opts, const char *path)
        {
            package_config config;

            if (config.load(path, opts))
            {
                return build(config, opts, path);
            }
            else
            {
                log_error("%s is not a valid prep package\n", path);
                return 1;
            }
        }
    }
}
