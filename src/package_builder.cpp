#include "package_builder.h"
#include "common.h"
#include "log.h"
#include "package_resolver.h"
#include "util.h"


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

        int package_builder::build_package(const package &config, const char *path)
        {
            string installPath, buildPath;

            if (!config.is_loaded()) {
                log_error("config not loaded");
                return PREP_FAILURE;
            }

            if (!force_build_ && repo_.has_meta(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
            }

            buildPath = repo_.get_build_path(config.name());

            if (!directory_exists(buildPath.c_str())) {
                log_trace("Creating [%s]", buildPath.c_str());
                if (mkpath(buildPath.c_str(), 0777)) {
                    log_errno(errno);
                    return PREP_FAILURE;
                }
            }

            log_trace("Setting build output to [%s]", buildPath.c_str());

            installPath = repo_.get_install_path(config.name());

            if (!directory_exists(installPath.c_str())) {
                log_trace("Creating %s...", installPath.c_str());
                if (mkpath(installPath.c_str(), 0777)) {
                    log_errno(errno);
                    return PREP_FAILURE;
                }
            }

            log_trace("Installing to [%s]", installPath.c_str());

            if (!str_cmp(config.build_system(), "cmake")) {
                if (build_cmake(config, path, buildPath.c_str(), installPath.c_str())) {
                    return PREP_FAILURE;
                }
            } else if (!str_cmp(config.build_system(), "autotools")) {
                if (build_autotools(config, path, buildPath.c_str(), installPath.c_str())) {
                    return PREP_FAILURE;
                }
            } else {
                auto cmds = config.build_commands();

                if (cmds.size() > 0) {
                    if (build_commands(config, path, cmds)) {
                        return PREP_FAILURE;
                    }
                } else {
                    log_error("unknown build system '%s'", config.build_system());
                    return PREP_FAILURE;
                }
            }

            if (repo_.save_meta(config)) {
                log_error("unable to save meta data");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int package_builder::remove(const package &config, options &opts)
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

            if (repo_.unlink_directory(installDir.c_str())) {
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

        int package_builder::build(const package &config, options &opts, const std::string &path)
        {
            string installDir;

            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            log_trace("Building from [%s]", path.c_str());

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

            for (package_dependency &p : config.dependencies()) {
                package_resolver resolver;
                string package_dir;

                log_info("    - preparing dependency \033[0;35m%s\033[0m", p.name());

                package_dir = repo_.exists_in_history(p.location());

                if (package_dir.empty() || !directory_exists(package_dir.c_str())) {
                    if (str_empty(p.location())) {
                        log_error("[%s] dependency [%s] has no location", config.name(), p.name());
                        continue;
                    }

                    if (resolver.resolve_package(p, opts, p.location())) {
                        return PREP_FAILURE;
                    }

                    package_dir = resolver.package_dir();
                }

                if (build(p, opts, package_dir)) {
                    log_error("unable to build dependency %s", p.name());
                    remove_directory(package_dir.c_str());
                    return PREP_FAILURE;
                }
            }

            if (repo_.has_meta(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
            }

            if (build_package(config, path.c_str())) {
                return PREP_FAILURE;
            }

            if (repo_.link_directory(installDir.c_str())) {
                log_error("Unable to link package");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int package_builder::link_package(const package &config) const
        {
            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            string packageInstall = repo_.get_install_path(config.name());

            return repo_.link_directory(packageInstall.c_str());
        }

        int package_builder::unlink_package(const package &config) const
        {
            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            string packageInstall = repo_.get_install_path(config.name());

            return repo_.unlink_directory(packageInstall.c_str());
        }

        int package_builder::execute(const package &config, int argc, char * const *argv) const
        {
            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            if (config.executable() == nullptr) {
                log_error("config has no executable defined");
                return PREP_FAILURE;
            }

            const char *executable = build_sys_path(repo_.get_bin_path().c_str(), config.executable(), NULL);

            if (!file_executable(executable)) {
                log_error("%s is not executable", executable);
                return PREP_FAILURE;
            }

            return repo_.execute(config.executable(), argc, argv);
        }

        int package_builder::build_cmake(const package &config, const char *path, const char *fromPath, const char *toPath)
        {
            char buf[BUFSIZ + 1] = {0};
            const char *buildopts = NULL;

            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            log_debug("Running cmake for \x1b[1;35m%s\x1b[0m", config.name());

            buildopts = config.build_options();

            if (buildopts == NULL) {
                buildopts = "";
            }

            snprintf(buf, BUFSIZ, "cmake -DCMAKE_INSTALL_PREFIX=%s %s %s", toPath, buildopts, path);

            if (env_.execute(buf, fromPath)) {
                log_error("unable to execute cmake");
                return PREP_FAILURE;
            }

            return build_make(config, fromPath);
        }

        int package_builder::build_make(const package &config, const char *path)
        {
            if (env_.execute("make -j2 install", path)) {
                log_error("unable to execute make");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int package_builder::build_commands(const package &config, const char *path, const vector<string> &commands)
        {
            for (auto &cmd : commands) {
                if (env_.execute(cmd.c_str(), path)) {
                    log_error("unable to execute command [%s]", cmd.c_str());
                    return PREP_FAILURE;
                }
            }

            return PREP_SUCCESS;
        }

        int package_builder::build_autotools(const package &config, const char *path, const char *fromPath, const char *toPath)
        {
            char buf[BUFSIZ + 1] = {0};
            const char *configure = NULL;
            const char *buildopts = NULL;

            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            log_debug("Running autotools for \x1b[1;35m%s\x1b[0m", config.name());

            buildopts = config.build_options();

            if (buildopts == NULL) {
                buildopts = "";
            }

            configure = build_sys_path(path, "configure", NULL);

            if (!file_exists(configure)) {
                const char *autogen = build_sys_path(path, "autogen.sh", NULL);

                if (!file_exists(autogen)) {
                    log_error("Don't know how to build %s... no autotools configuration found.", config.name());
                    return PREP_FAILURE;
                }

                log_debug("Executing %s in %s", autogen, path);

                // for fork, autogen points to a static variable
                strcpy(buf, autogen);

                if (env_.execute(buf, path)) {
                    log_error("unable to execute configure");
                    return PREP_FAILURE;
                }

                configure = build_sys_path(path, "configure", NULL);

                if (!file_exists(configure)) {
                    log_error("Could not generate a configure script for %s", config.name());
                    return PREP_FAILURE;
                }
            }

            snprintf(buf, BUFSIZ, "%s --prefix=%s %s", configure, toPath, buildopts);

            log_debug("Executing %s in %s", buf, fromPath);

            if (env_.execute(buf, fromPath)) {
                log_error("unable to execute configure");
                return PREP_FAILURE;
            }

            return build_make(config, fromPath);
        }

        int package_builder::build_from_folder(options &opts, const char *path)
        {
            package_config config;

            if (config.load(path, opts)) {
                return build(config, opts, path);
            } else {
                log_error("%s is not a valid prep package\n", path);
                return 1;
            }
        }
    }
}
