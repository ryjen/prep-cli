#include "package_builder.h"
#include "common.h"
#include "log.h"
#include "util.h"

namespace micrantha
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
                if (repo_.validate_plugins(opts) == PREP_FAILURE) {
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

        prep::repository *package_builder::repository()
        {
            return &repo_;
        }

        void package_builder::add_path_to_shell() const
        {
            prompt_to_add_path_to_shell_rc(".zshrc", repo_.get_bin_path().c_str());
            if (prompt_to_add_path_to_shell_rc(".bash_profile", repo_.get_bin_path().c_str())) {
                prompt_to_add_path_to_shell_rc(".bashrc", repo_.get_bin_path().c_str());
            }
            prompt_to_add_path_to_shell_rc(".kshrc", repo_.get_bin_path().c_str());
        }

        int package_builder::build_package(const package &config, const char *path)
        {
            std::string installPath, buildPath;

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

            if (repo_.notify_plugins_build(config, path, buildPath, installPath) == PREP_FAILURE) {
                log_error("unable to build [%s]", config.name().c_str());
                return PREP_FAILURE;
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
            if (repo_.notify_plugins_remove(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
            }

            return remove(config.name(), opts);
        }

        int package_builder::remove(const std::string &package_name, options &opts)
        {
            std::string installDir = repo_.get_install_path(package_name);

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
            std::string installDir;

            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            log_trace("Building from [%s]", path);

            installDir = repo_.get_install_path(config.name());

            if (!directory_exists(installDir.c_str())) {
                if (mkpath(installDir.c_str(), 0777)) {
                    log_error("could not create [%s] (%s)", installDir.c_str(), strerror(errno));
                    return PREP_FAILURE;
                }
            }

            for (package_dependency &p : config.dependencies()) {
                std::string package_dir;
                char buf[PATH_MAX];

                log_info("preparing dependency \033[0;35m%s\033[0m", p.name().c_str());

                if (repo_.notify_plugins_install(p) == PREP_SUCCESS) {
                    continue;
                }

                auto callback = [&package_dir](const plugin::Result &result) {
                    package_dir = result.values.front();
                };

                if (repo_.notify_plugins_resolve(p, callback) != PREP_SUCCESS || package_dir.empty()) {
                    log_error("[%s] could not resolve dependency [%s]", config.name().c_str(), p.name().c_str());
                    return PREP_FAILURE;
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

            std::string packageInstall = repo_.get_install_path(config.name());

            return repo_.link_directory(packageInstall.c_str());
        }

        int package_builder::unlink_package(const package &config) const
        {
            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return PREP_FAILURE;
            }

            std::string packageInstall = repo_.get_install_path(config.name());

            return repo_.unlink_directory(packageInstall.c_str());
        }

        int package_builder::execute(const package &config, int argc, char *const *argv) const
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
    }
}
