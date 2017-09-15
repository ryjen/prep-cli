#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sstream>
#include <vector>
#ifdef HAVE_CURL_CURL_H
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif


#include "common.h"
#include "log.h"
#include "package_config.h"
#include "plugin.h"
#include "repository.h"
#include "util.h"

namespace micrantha
{
    namespace prep
    {
        package::package() : values_()
        {
        }

        package::package(const json_type &obj) : values_(obj)
        {
            init_dependencies();
            init_build_system();
        }

        void package::init_dependencies()
        {
            auto deps = values_["dependencies"];

            if (!deps.is_array()) {
                return;
            }

            for (auto &dep : deps) {
                dependencies_.push_back(package_dependency(dep));
            }
        }


        int package::dependency_count(const std::string &package_name) const
        {
            int count = 0;

            for (const package_dependency &dep : dependencies()) {
                if (!strcmp(dep.name().c_str(), package_name.c_str())) {
                    count++;
                }
                count += dep.dependency_count(package_name);
            }

            return count;
        }


        void package::init_build_system()
        {
            if (values_.count("build_system")) {
                std::vector<std::string> systems = values_["build_system"];
                for (auto &system : systems) {
                    build_system_.push_back(system);
                }
            }
        }

        package::package(const package &other)
            : values_(other.values_),
              path_(other.path_),
              dependencies_(other.dependencies_),
              build_system_(other.build_system_)
        {
        }

        package &package::operator=(const package &other)
        {
            values_       = other.values_;
            path_         = other.path_;
            dependencies_ = other.dependencies_;
            build_system_ = other.build_system_;
            return *this;
        }

        package::~package()
        {
        }

        package_config::package_config()
        {
        }

        package_config::package_config(const package_config &other) : package(other)
        {
        }

        package_config::~package_config()
        {
        }

        package_config &package_config::operator=(const package_config &other)
        {
            package::operator=(other);

            return *this;
        }

        bool package::is_loaded() const
        {
            return !values_.is_null();
        }

        std::vector<package_dependency> package::dependencies() const
        {
            return dependencies_;
        }

        int package_config::resolve_package_file(const std::string &path, const std::string &filename,
                                                 std::ifstream &file)
        {
            char buf[PATH_MAX] = { 0 };

            snprintf(buf, PATH_MAX, "%s/%s", path.c_str(), filename.c_str());

            if (file_exists(buf)) {
                file.open(buf);

                return PREP_SUCCESS;
            }

            if (filename.find("://") != std::string::npos) {
                return download_package_file(path, filename, file);
            }

            return PREP_FAILURE;
        }

        int package_config::download_package_file(const std::string &path, const std::string &url, std::ifstream &file)
        {
            std::string buffer;
            char filename[PATH_MAX + 1] = { 0 };
            char *fname                 = NULL;

            log_debug("attempting download of %s", url.c_str());

            if (download_to_temp_file(url.c_str(), buffer)) {
                log_error("unable to download %s", url.c_str());
                return PREP_FAILURE;
            }

            strncpy(filename, url.c_str(), BUFSIZ);

            fname = basename(filename);

            snprintf(filename, PATH_MAX, "%s/%s", path.c_str(), fname);

            if (rename(buffer.c_str(), filename)) {
                log_error("unable to rename %s to %s", buffer.c_str(), filename);
                return PREP_FAILURE;
            }

            file.open(filename);

            return PREP_SUCCESS;
        }

        int package_config::load(const std::string &path, const options &opts)
        {
            std::ifstream file;
            std::ostringstream os;

            if (path.empty()) {
                return PREP_FAILURE;
            }

            if (resolve_package_file(path, opts.package_file, file)) {
                log_debug("unable to resolve %s/%s", path.c_str(), opts.package_file.c_str());
                return PREP_FAILURE;
            }

            if (!file.is_open()) {
                log_error("unable to open %s", opts.package_file.c_str());
                return PREP_FAILURE;
            }

            file >> os.rdbuf();

            values_ = json_type::parse(os.str().c_str());

            if (values_.size() == 0) {
                printf("invalid configuration for %s\n", os.str().c_str());
                return PREP_FAILURE;
            }

            init_dependencies();

            init_build_system();

            path_ = build_sys_path(path.c_str(), opts.package_file.c_str(), nullptr);

            log_info("Preparing package \033[1;35m%s\033[0m", name().c_str());

            return PREP_SUCCESS;
        }

        std::string package::name() const
        {
            auto name = values_.find("name");
            if (name == values_.end()) {
                return "";
            }
            return *name;
        }

        std::string package::version() const
        {
            auto vers = values_.find("version");
            if (vers == values_.end()) {
                return "";
            }
            return *vers;
        }

        std::vector<std::string> package::build_system() const
        {
            return build_system_;
        }

        std::string package::location() const
        {
            auto loc = values_.find("location");
            if (loc == values_.end()) {
                return "";
            }
            return *loc;
        }

        std::string package::build_options() const
        {
            return values_["build_options"];
        }

        std::string package::executable() const
        {
            return values_["executable"];
        }

        std::string package::path() const
        {
            return path_;
        }

        bool package::has_path() const
        {
            return !path_.empty();
        }

        bool package::has_name() const
        {
            return values_.count("name");
        }

        package::json_type package::get_plugin_config(const plugin *plugin) const
        {
            if (plugin == nullptr) {
                return json_type();
            }

            auto config = values_.find(plugin->name());

            if (config == values_.end()) {
                return json_type();
            }
            return *config;
        }

        package_dependency::package_dependency(const package::json_type &obj) : package(obj)
        {
        }

        package_dependency::package_dependency(const package_dependency &other) : package(other)
        {
        }

        package_dependency::~package_dependency()
        {
        }

        int package_dependency::load(const std::string &path, const options &opts)
        {
            return PREP_SUCCESS;
        }
    }
}
