#include "package_builder.h"
#include "common.h"
#include "log.h"
#include "util.h"
#include "vt100.h"

namespace micrantha
{
    namespace prep
    {
        PackageBuilder::PackageBuilder() = default;

        int PackageBuilder::initialize(const Options &opts) {
            if (repo_.initialize(opts)) {
                return PREP_FAILURE;
            }

            if (repo_.validate(opts) == PREP_FAILURE) {
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int PackageBuilder::load(const Options &opts) {

            vt100::Progress progress;

            if (repo_.validate_plugins(opts) == PREP_FAILURE) {
                return PREP_FAILURE;
            }

            if (repo_.load_plugins(opts) == PREP_FAILURE) {
                log::error("unable to load plugins");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        Repository *PackageBuilder::repository()
        {
            return &repo_;
        }

        void PackageBuilder::print_env() const
        {
            auto envVars = environment::build_cpp_variables();

            vt100::print("\n", color::g("ENV"), ":\n\n");

            for(const auto &entry : envVars) {
               vt100::print(color::c(entry.first), "=", color::w(entry.second), "\n\n");
            }
        }

        int PackageBuilder::build_package(const Package &config, const Options &opts, const char *path)
        {
            std::string installPath, buildPath;

            if (!config.is_loaded()) {
                log::error("config not loaded");
                return PREP_FAILURE;
            }

            if (!opts.force_build && repo_.has_meta(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
            }

            buildPath = repo_.get_build_path(config.name());

            if (!directory_exists(buildPath.c_str())) {
                log::trace("Creating [", buildPath, "]");
                if (mkpath(buildPath.c_str(), 0777)) {
                    log::perror(errno);
                    return PREP_FAILURE;
                }
            }

            log::trace("Setting build output to [", buildPath, "]");

            installPath = repo_.get_install_path(config.name());

            if (!directory_exists(installPath.c_str())) {
                log::trace("Creating ", installPath, "...");
                if (mkpath(installPath.c_str(), 0777)) {
                    log::perror(errno);
                    return PREP_FAILURE;
                }
            }

            log::trace("Installing to [", installPath, "]");

            if (repo_.notify_plugins_build(config, path, buildPath, installPath) == PREP_FAILURE) {
                log::error("unable to build [", config.name(), "]");
                return PREP_FAILURE;
            }

            if (repo_.save_meta(config)) {
                log::error("unable to save meta data");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        std::string PackageBuilder::get_package_directory(const std::string &name) const
        {
            return repo_.get_meta_path(name);
        }

        int PackageBuilder::remove(const Package &config, const Options &opts)
        {
            // otherwise were removing this package
            if (!config.is_loaded()) {
                log::error("configuration not loaded");
                return PREP_FAILURE;
            }

            // TODO: ensure we actually installed via plugin
            if (repo_.notify_plugins_remove(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
            }

            // remove the directory structure
            return remove(config.name(), opts);
        }

        int PackageBuilder::remove(const std::string &package_name, const Options &opts)
        {
            std::string installDir = repo_.get_install_path(package_name);

            if (!directory_exists(installDir.c_str())) {
                log::info(color::m(package_name), " is not installed");
                return PREP_SUCCESS;
            }

            int depCount = repo_.dependency_count(package_name, opts);

            if (depCount > 0) {
                log::warn(depCount, " packages depend on ", package_name);
                return PREP_FAILURE;
            }

            if (repo_.unlink_directory(installDir.c_str())) {
                log::error("unable to unlink package ", package_name);
                return PREP_FAILURE;
            }

            if (remove_directory(installDir.c_str())) {
                log::error("unable to remove package ", installDir);
                return PREP_FAILURE;
            }

            installDir = repo_.get_meta_path(package_name);

            if (remove_directory(installDir.c_str())) {
                log::error("unable to remove meta package ", installDir);
                return PREP_FAILURE;
            }

            log::info(color::m(package_name), " package removed.");

            return PREP_SUCCESS;
        }

        int PackageBuilder::build(const Package &config, const Options &opts, const char *path)
        {
            std::string installDir;

            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            vt100::Progress progress;

            log::info("preparing package ", color::m(config.name()), " [", color::y(config.version()), "]");

            if (!opts.force_build && repo_.has_meta(config) == PREP_SUCCESS) {
                log::warn("used cached version of ", color::m(config.name()), " [", color::y(config.version()), "]");
                return PREP_SUCCESS;
            }

            installDir = repo_.get_install_path(config.name());

            if (!directory_exists(installDir.c_str())) {
                if (mkpath(installDir.c_str(), 0777)) {
                    log::error("could not create [", installDir, "] (", strerror(errno), ")");
                    return PREP_FAILURE;
                }
            }

            for (const auto &p : config.dependencies()) {
                std::string package_dir;

                log::info("preparing ", color::m(config.name()), " dependency ", color::c(p.name()), " [",
                          color::y(p.version()), "]");

                if (repo_.notify_plugins_install(p) == PREP_SUCCESS) {
                    continue;
                }

                auto callback = [&package_dir](const Plugin::Result &result) { package_dir = result.values.front(); };

                if (repo_.notify_plugins_resolve(p, callback) != PREP_SUCCESS || package_dir.empty()) {
                    log::error("[", config.name(), "] could not resolve dependency [", p.name(), "]");
                    return PREP_FAILURE;
                }

                if (build(p, opts, package_dir.c_str())) {
                    log::error("unable to build dependency ", p.name());
                    remove_directory(package_dir.c_str());
                    return PREP_FAILURE;
                }
            }

            if (build_package(config, opts, path)) {
                return PREP_FAILURE;
            }

            if (repo_.link_directory(installDir)) {
                log::error("Unable to link package");
                return PREP_FAILURE;
            }

            log::info("built package ", color::m(config.name()), " [", color::y(config.version()), "]");

            return PREP_SUCCESS;
        }

        int PackageBuilder::link_package(const Package &config) const
        {
            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            auto packageInstall = repo_.get_install_path(config.name());

            return repo_.link_directory(packageInstall);
        }

        int PackageBuilder::unlink_package(const Package &config) const
        {
            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            std::string packageInstall = repo_.get_install_path(config.name());

            return repo_.unlink_directory(packageInstall);
        }

        int PackageBuilder::execute(const Package &config, int argc, char *const *argv) const
        {
            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            if (config.executable().empty()) {
                log::error("config has no executable defined");
                return PREP_FAILURE;
            }

            auto executable = build_sys_path(repo_.get_bin_path().c_str(), config.executable().c_str(), NULL);

            if (!file_executable(executable)) {
                log::error(executable, " is not executable");
                return PREP_FAILURE;
            }

            return repo_.execute(config.executable(), argc, argv);
        }
    }
}
