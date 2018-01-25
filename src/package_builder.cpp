#include <unistd.h>
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

        int PackageBuilder::initialize(const Options &opts)
        {
            if (repo_.initialize(opts)) {
                return PREP_FAILURE;
            }

            if (repo_.validate(opts) == PREP_FAILURE) {
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int PackageBuilder::load(const Options &opts)
        {
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
            auto envVars = environment::build_map();

            for (const auto &entry : envVars) {
                vt100::print(color::c(entry.first), "=", color::w(entry.second), "\n");
            }
        }

        int PackageBuilder::build_package(const Package &config, const Options &opts, const std::string &path)
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

            if (directory_exists(buildPath) != PREP_SUCCESS) {
                log::trace("Creating [", buildPath, "]");
                if (mkpath(buildPath, 0777)) {
                    log::perror(errno);
                    return PREP_FAILURE;
                }
            }

            installPath = repo_.get_install_path(config.name());

            if (directory_exists(installPath) != PREP_SUCCESS) {
                log::trace("Creating [", installPath, "]");
                if (mkpath(installPath, 0777)) {
                    log::perror(errno);
                    return PREP_FAILURE;
                }
            }
            log::trace("Setting build output to [", installPath, "]");

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


        int PackageBuilder::test_package(const Package &config, const std::string &path)
        {
            std::string installPath, buildPath;

            if (!config.is_loaded()) {
                log::error("config not loaded");
                return PREP_FAILURE;
            }

            if (repo_.has_meta(config) != PREP_SUCCESS) {
                log::error("unable to test ", config.name(), ", try building first");
                return PREP_SUCCESS;
            }

            buildPath = repo_.get_build_path(config.name());

            if (directory_exists(buildPath) != PREP_SUCCESS) {
                log::perror("No build path to test for ", config.name());
                return PREP_FAILURE;
            }

            if (repo_.notify_plugins_test(config, path, buildPath) == PREP_FAILURE) {
                log::error("unable to test [", config.name(), "]");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }


        int PackageBuilder::install_package(const Package &config, const std::string &path)
        {
            std::string buildPath;

            if (!config.is_loaded()) {
                log::error("config not loaded");
                return PREP_FAILURE;
            }

            if (repo_.has_meta(config) != PREP_SUCCESS) {
                log::error("unable to install ", config.name(), ", try building first");
                return PREP_SUCCESS;
            }

            buildPath = repo_.get_build_path(config.name());

            if (directory_exists(buildPath) != PREP_SUCCESS) {
                log::error("No build path to install ", config.name());
                return PREP_FAILURE;
            }

            if (repo_.notify_plugins_install(config, path, buildPath) == PREP_FAILURE) {
                log::error("unable to build [", config.name(), "]");
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

				int PackageBuilder::cleanup(const Package &config, const Options &opts) {

								auto buildDir = repo_.get_build_path(config.name());

								if (directory_exists(buildDir) == PREP_SUCCESS) {
											if (remove_directory(buildDir) == PREP_FAILURE) {
															log::error("unable to clean ", buildDir);
															return PREP_FAILURE;
											}
											return PREP_SUCCESS;
								}
								log::error("Already clean!");
								return PREP_FAILURE;
				}
        int PackageBuilder::remove(const std::string &package_name, const Options &opts)
        {
            std::string installDir = repo_.get_install_path(package_name);

            if (directory_exists(installDir) != PREP_SUCCESS) {
                log::info(color::m(package_name), " is not installed");
                return PREP_SUCCESS;
            }

            int depCount = repo_.dependency_count(package_name, opts);

            if (depCount > 0) {
                log::warn(depCount, " packages depend on ", package_name);
                return PREP_FAILURE;
            }

            if (repo_.unlink_directory(installDir)) {
                log::error("unable to unlink package ", package_name);
                return PREP_FAILURE;
            }

            if (remove_directory(installDir)) {
                log::error("unable to remove package ", installDir);
                return PREP_FAILURE;
            }

            installDir = repo_.get_meta_path(package_name);

            if (remove_directory(installDir)) {
                log::error("unable to remove meta package ", installDir);
                return PREP_FAILURE;
            }

            log::info(color::m(package_name), " package removed.");

            return PREP_SUCCESS;
        }

        int PackageBuilder::build(const Package &config, const Options &opts, const std::string &path) {

            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            vt100::Progress progress;

            log::info("preparing package ", color::m(config.name()), " [", color::y(config.version()), "]");

            for (const auto &c : config.dependencies()) {
                std::string package_dir;

                if (!opts.force_build && repo_.exists(c)) {
                    log::info("using cached version of ", color::m(config.name()), " dependency ", color::c(c.name()),
                        " [", color::y(c.version()),  "]");
                    continue;
                }

                log::info("preparing ", color::m(config.name()), " dependency ", color::c(c.name()), " [",
                          color::y(c.version()), "]");

                // try to add via plugin
                if (repo_.notify_plugins_add(c) == PREP_SUCCESS) {

                    if (repo_.save_meta(c)) {
                        log::warn("unable to save meta data for ", config.name());
                    }
                    continue;
                }

                // then try to resolve the source
                auto result = repo_.notify_plugins_resolve(c);

                if (result != PREP_SUCCESS || result.values.empty()) {
                    log::error("[", config.name(), "] could not resolve dependency [", c.name(), "]");
                    return PREP_FAILURE;
                }

                package_dir = result.values.front();

                // build the dependency source
                if (build(c, opts, package_dir) != PREP_SUCCESS) {
                    log::error("unable to build dependency ", c.name());
                    return PREP_FAILURE;
                }

                if (install(c, opts) != PREP_SUCCESS) {
                    log::error("unable to install dependency ", c.name());
                    return PREP_FAILURE;
                }
            }

            if (!opts.force_build && repo_.exists(config)) {
                log::warn("used cached version of ", color::m(config.name()), " [", color::y(config.version()), "]");
                return PREP_SUCCESS;
            }

            return build_package(config, opts, path);
        }

        int PackageBuilder::test(const Package &config, const Options &opts) {

            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            if (repo_.has_meta(config) != PREP_SUCCESS) {
                log::error("Cannot test ", config.name(), " try building first");
                return PREP_FAILURE;
            }

            vt100::Progress progress;

            log::info("testing package ", color::m(config.name()), " [", color::y(config.version()), "]");

            auto sourcePath = repo_.get_source_path(config.name());

            return test_package(config, sourcePath);
        }

        int PackageBuilder::install(const Package &config, const Options &opts) {

            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            if (repo_.has_meta(config) != PREP_SUCCESS) {
                log::error("Cannot install ", config.name(), " try building first");
                return PREP_FAILURE;
            }

            vt100::Progress progress;

            log::info("installing package ", color::m(config.name()), " [", color::y(config.version()), "]");

            auto sourcePath = repo_.get_install_path(config.name());

            if (install_package(config, sourcePath)) {
                return PREP_FAILURE;
            }

            auto installPath = repo_.get_install_path(config.name());

            if (repo_.link_directory(installPath)) {
                log::error("Unable to link package");
                return PREP_FAILURE;
            }

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

            auto executable = build_sys_path(repo_.get_bin_path(), config.executable());

            if (!file_executable(executable)) {
                log::error(executable, " is not executable");
                return PREP_FAILURE;
            }

            int err = system(executable.c_str());

            switch (err) {
                case -1:
                return PREP_ERROR;
                case 127:
                    return PREP_FAILURE;
                default:
                    return PREP_SUCCESS;
            }
        }
    }
}
