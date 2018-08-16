#include <vector>
#include <unistd.h>
#include <limits.h>

#include "controller.h"
#include "common.h"
#include "log.h"
#include "util.h"
#include "plugin_manager.h"

namespace micrantha {
    namespace prep {
        Controller::Controller() = default;

        int Controller::initialize(const Options &opts) {
            if (repo_.initialize(opts)) {
                return PREP_FAILURE;
            }

            if (repo_.validate(opts) == PREP_FAILURE) {
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int Controller::load(const Options &opts) {


            if (repo_.load_plugins(opts) == PREP_FAILURE) {
                log::error("unable to load plugins");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        Repository *Controller::repository() {
            return &repo_;
        }

        int Controller::print_env(char *const var) const {

            // a single argument named prefix will print
            // the current repository path
            if (var && !strcasecmp(var, "prefix")) {
                auto temp = Repository::get_local_repo();

                if (filesystem::directory_exists(temp) == PREP_SUCCESS) {
                    io::print(temp, "\n");
                    return PREP_SUCCESS;
                }

                if (filesystem::directory_exists(Repository::GLOBAL_REPO) == PREP_SUCCESS) {
                    io::print(Repository::GLOBAL_REPO);
                    return PREP_SUCCESS;
                }
                return PREP_FAILURE;
            }

            // the list of environment variables prep uses for the current repository
            auto envVars = environment::build_map();

            for (const auto &entry : envVars) {

                if (!var) {
                    // print all
                    io::print(entry.first, "=", entry.second, "\n");
                } else if (!strcasecmp(var, entry.first.c_str())) {
                    // print a single value
                    io::print(entry.second, "\n");
                    return PREP_SUCCESS;
                }
            }
            return !var && !envVars.empty();
        }

        int Controller::build_package(const Package &config, const Options &opts, const std::string &path) {
            std::string installPath, buildPath;
            char sourcePath[PATH_MAX] = {0};

            if (!config.is_loaded()) {
                log::error("config not loaded");
                return PREP_FAILURE;
            }

            if (opts.force_build == ForceLevel::None && repo_.has_meta(config) == PREP_SUCCESS) {
                return PREP_SUCCESS;
            }

            buildPath = repo_.get_build_path(config.name());

            if (filesystem::directory_exists(buildPath) != PREP_SUCCESS) {
                log::trace("Creating [", buildPath, "]");
                if (filesystem::create_path(buildPath)) {
                    log::perror(errno);
                    return PREP_FAILURE;
                }
            }

            installPath = repo_.get_install_path(config.name());

            if (filesystem::directory_exists(installPath) != PREP_SUCCESS) {
                log::trace("Creating [", installPath, "]");
                if (filesystem::create_path(installPath)) {
                    log::perror(errno);
                    return PREP_FAILURE;
                }
            }

            if (!realpath(path.c_str(), sourcePath)) {
                log::error("unable to find path for ", path);
                return PREP_FAILURE;
            }

            log::trace("source[", sourcePath, "], build[", buildPath, "], install[", installPath, "]");

            if (repo_.notify_plugins_build(config, sourcePath, buildPath, installPath) == PREP_FAILURE) {
                log::error("unable to build [", config.name(), "]");
                return PREP_FAILURE;
            }

            if (repo_.save_meta(config)) {
                log::error("unable to save meta data");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }


        int Controller::test_package(const Package &config, const std::string &path) {
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

            if (filesystem::directory_exists(buildPath) != PREP_SUCCESS) {
                log::perror("No build path to test for ", config.name());
                return PREP_FAILURE;
            }

            if (repo_.notify_plugins_test(config, path, buildPath) == PREP_FAILURE) {
                log::error("unable to test [", config.name(), "]");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int Controller::install_package(const Package &config, const std::string &path) {
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

            if (filesystem::directory_exists(buildPath) != PREP_SUCCESS) {
                log::error("No build path to install ", config.name());
                return PREP_FAILURE;
            }

            if (repo_.notify_plugins_install(config, path, buildPath) == PREP_FAILURE) {
                log::error("unable to build [", config.name(), "]");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        std::string Controller::get_package_directory(const std::string &name) const {
            return repo_.get_meta_path(name);
        }

        int Controller::remove(const Package &config, const Options &opts) {
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

        int Controller::cleanup(const Package &config, const Options &opts) {

            auto buildDir = repo_.get_build_path(config.name());

            if (filesystem::directory_exists(buildDir) == PREP_SUCCESS) {
                if (filesystem::remove_directory(buildDir) == PREP_FAILURE) {
                    log::error("unable to clean ", buildDir);
                    return PREP_FAILURE;
                }
                return PREP_SUCCESS;
            }
            log::error("Already clean!");
            return PREP_FAILURE;
        }

        int Controller::remove(const std::string &package_name, const Options &opts) {
            std::string installDir = repo_.get_install_path(package_name);

            if (filesystem::directory_exists(installDir) != PREP_SUCCESS) {
                log::info(color::m(package_name), " is not installed");
                return PREP_SUCCESS;
            }

            if (opts.force_build == ForceLevel::None) {
                int depCount = repo_.dependency_count(package_name, opts);

                if (depCount > 0) {
                    log::warn(depCount, " packages depend on ", package_name);
                    return PREP_FAILURE;
                }
            }

            if (repo_.unlink_directory(installDir)) {
                log::error("unable to unlink package ", package_name);
                return PREP_FAILURE;
            }

            if (filesystem::remove_directory(installDir)) {
                log::error("unable to remove package ", installDir);
                return PREP_FAILURE;
            }

            installDir = repo_.get_meta_path(package_name);

            if (filesystem::remove_directory(installDir)) {
                log::error("unable to remove meta package ", installDir);
                return PREP_FAILURE;
            }

            log::info(color::m(package_name), " package removed.");

            return PREP_SUCCESS;
        }

        int Controller::build(const Package &config, const Options &opts, const std::string &path) {

            // get the dependencies first
            if (get(config, opts, path) != PREP_SUCCESS) {
                return PREP_FAILURE;
            }


            if (opts.force_build == ForceLevel::None && repo_.exists(config)) {
                log::warn("used cached version of ", color::m(config.name()), " [", color::y(config.version()), "]");
                return PREP_SUCCESS;
            }

            int rval = build_package(config, opts, path);

            log::info("done building ", config.name());

            return rval;
        }

        int Controller::get(const Package &config, const Options &opts, const std::string &path) {

            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            log::info("preparing package ", color::m(config.name()), " [", color::y(config.version()), "]");

            for (const auto &c : config.dependencies()) {

                if (opts.force_build != ForceLevel::All && repo_.exists(c)) {
                    log::info("using cached version of ", color::m(config.name()), " dependency ", color::c(c.name()),
                            " [", color::y(c.version()), "]");
                    continue;
                }

            log::info("preparing ", color::m(config.name()), " dependency ", color::c(c.name()), " [",
                    color::y(c.version()), "]");

                if (get_package(c, opts, path) == PREP_FAILURE) {
                    return PREP_FAILURE;
                }
            }

            if (dynamic_cast<const PackageDependency*>(&config)) {
                return get_package(dynamic_cast<const PackageDependency&>(config), opts, path);
            }

            return PREP_SUCCESS;
        }

        int Controller::get_package(const PackageDependency &config, const Options &opts, const std::string &path) {

            // try to add via plugin
            if (repo_.notify_plugins_add(config) == PREP_SUCCESS) {

                if (repo_.save_meta(config)) {
                    log::warn("unable to save meta data for ", config.name());
                }
                return PREP_SUCCESS;
            }

            // then try to resolve the source
            auto result = repo_.notify_plugins_resolve(config);

            if (result != PREP_SUCCESS || result.values.empty()) {
                log::error("[", config.name(), "] could not resolve dependency [", config.name(), "]");
                return PREP_FAILURE;
            }

            auto package_dir = result.values.front();

            // build the dependency source
            if (build_package(config, opts, package_dir) != PREP_SUCCESS) {
                log::error("unable to build dependency ", config.name());
                return PREP_FAILURE;
            }

            if (install(config, opts) != PREP_SUCCESS) {
                log::error("unable to install dependency ", config.name());
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int Controller::test(const Package &config, const Options &opts) {

            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            if (repo_.has_meta(config) != PREP_SUCCESS) {
                log::error("Cannot test ", config.name(), " try building first");
                return PREP_FAILURE;
            }

            log::info("testing package ", color::m(config.name()), " [", color::y(config.version()), "]");

            auto sourcePath = repo_.get_source_path(config.name());

            return test_package(config, sourcePath);
        }

        int Controller::install(const Package &config, const Options &opts) {

            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            if (repo_.has_meta(config) != PREP_SUCCESS) {
                log::error("Cannot install ", config.name(), " try building first");
                return PREP_FAILURE;
            }

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

        int Controller::link(const Package &config) const {
            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            auto packageInstall = repo_.get_install_path(config.name());

            return repo_.link_directory(packageInstall);
        }

        int Controller::unlink(const Package &config) const {
            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            std::string packageInstall = repo_.get_install_path(config.name());

            return repo_.unlink_directory(packageInstall);
        }

        int Controller::execute(const Package &config, int argc, char *const *argv) const {
            if (!config.is_loaded()) {
                log::error("config is not loaded");
                return PREP_FAILURE;
            }

            if (config.executable().empty()) {
                log::error("config has no executable defined");
                return PREP_FAILURE;
            }

            auto executable = filesystem::build_path(repo_.get_bin_path(), config.executable());

            if (!filesystem::is_file_executable(executable)) {
                log::error(executable, " is not executable");
                return PREP_FAILURE;
            }

            // build arg list crap
            const char* args[argc+1];

            auto fname = config.executable();

            args[0] = fname.c_str();

            for (int i = 1; i < argc; i++) {
                args[i] = argv[i];
            }

            args[argc] = 0;

            auto runEnv = environment::run_env();

            const char *envp[runEnv.size()+1];

            for (int i = 0; i < runEnv.size(); i++) {
                envp[i] = runEnv[i].c_str();
            }

            envp[runEnv.size()] = 0;

            execve(executable.c_str(), const_cast<char**>(args), const_cast<char**>(envp));

            log::error("unable to run ", executable);

            return PREP_FAILURE;
        }

        int Controller::plugins(const Options &opts, int argc, char *const *argv) {

            PluginManager manager(repo_);

            return manager.execute(opts, argc, argv);
        }
    }
}
