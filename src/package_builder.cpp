#include "package_builder.h"
#include "common.h"
#include "log.h"
#include "package_resolver.h"
#include "util.h"
#include "exception.h"

namespace rj
{
    namespace prep
    {
        int package_builder::initialize(const options &opts)
        {
            if (repo_.initialize(opts)) {
                return PREP_FAILURE;
            }

            if (repo_.validate() == PREP_FAILURE) {
                return PREP_FAILURE;
            }

            if (!opts.global) {
                if (repo_.validate_plugins() == PREP_FAILURE) {
                    return PREP_FAILURE;
                }
            }

            if (repo_.load_plugins() == PREP_FAILURE) {
                log_error("unable to load plugins");
                return PREP_FAILURE;
            }

            force_build_ = opts.force_build;

            return PREP_SUCCESS;
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

            if (repo_.plugin_build(config, path, buildPath, installPath) == PREP_FAILURE)
            {
                auto cmds = config.build_system();

                if (cmds.size() > 0) {
                    if (build_commands(config, path, cmds)) {
                        return PREP_FAILURE;
                    }
                } else {
                    return PREP_FAILURE;
                }
            }

            if (repo_.save_meta(config)) {
                log_error("unable to save meta data");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int package_builder::remove(package &config, options &opts, const char *package)
        {
            // otherwise were removeing this package
            if (!config.is_loaded()) {
                std::string directory = repo_.get_meta_path(package);

                log_trace("resolving package %s...", directory.c_str());

                if (config.load(directory, opts) == PREP_FAILURE) {
                    log_error("unable to load package configuration");
                    return PREP_FAILURE;
                }
            }

            // TODO: ensure we actually installed via plugin
            if (repo_.plugin_remove(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
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

        int package_builder::build(const package &config, options &opts, const char *path)
        {
            string installDir;

            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            log_trace("Building from [%s]", path);

            repo_.save_history(config.location(), path);

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

                log_info("    - preparing dependency \033[0;35m%s\033[0m", p.name().c_str());

                if (repo_.plugin_install(p) == PREP_SUCCESS) {
                    continue;
                }

                package_dir = repo_.plugin_resolve(p);

                if (package_dir.empty()) {
                    package_dir = repo_.exists_in_history(p.location());
                }

                if (package_dir.empty() || !directory_exists(package_dir.c_str())) {

                    if (p.location().empty()) {
                        log_error("[%s] dependency [%s] has no plugin or location", config.name().c_str(), p.name().c_str());
                        return PREP_FAILURE;
                    }

                    if (resolver.resolve_package(p, opts, p.location())) {
                        return PREP_FAILURE;
                    }

                    package_dir = resolver.package_dir();
                }

                if (build(p, opts, package_dir.c_str())) {
                    log_error("unable to build dependency %s", p.name().c_str());
                    remove_directory(package_dir.c_str());
                    return PREP_FAILURE;
                }
            }

            if (repo_.has_meta(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
            }

            if (build_package(config, path)) {
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

            if (config.executable().empty()) {
                log_error("config has no executable defined");
                return PREP_FAILURE;
            }

            auto executable = build_sys_path(repo_.get_bin_path().c_str(), config.executable().c_str(), NULL);

            if (!file_executable(executable)) {
                log_error("%s is not executable", executable);
                return PREP_FAILURE;
            }

            return repo_.execute(config.executable().c_str(), argc, argv);
        }

        int package_builder::build_commands(const package &config, const char *path, const vector<string> &commands)
        {
            for (auto &cmd : commands) {
                if (environment::execute(cmd.c_str(), path)) {
                    log_error("unable to execute command [%s]", cmd.c_str());
                    return PREP_FAILURE;
                }
            }

            return PREP_SUCCESS;
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
