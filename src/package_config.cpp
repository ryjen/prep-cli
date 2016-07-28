
#include "package_config.h"
#include <fstream>
#include <sstream>
#include "common.h"
#include "log.h"
#include "util.h"
#ifdef HAVE_CURL_CURL_H
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include "plugin.h"
#include "array.h"

namespace rj
{
    namespace prep
    {
        static const char *const DEFAULT_LOCATION = ".";

        static const char *const PACKAGE_FILE = "package.json";

        options::options_s() : global(false), package_file(PACKAGE_FILE), location(DEFAULT_LOCATION), force_build(false)
        {
        }

        package::package() : values_()
        {
        }

        package::package(const rj::json::object &obj) : values_(obj)
        {
            if (values_.contains("dependencies")) {
                auto deps = values_.get_array("dependencies");

                for (auto &dep: deps) {
                    dependencies_.push_back(package_dependency(dep));
                }
            }
        }

        package::package(const package &other) : values_(other.values_)
        {
            for (const auto &dep : other.dependencies_) {
                dependencies_.push_back(dep);
            }
        }

        package &package::operator=(const package &other)
        {
            values_ = other.values_;
            for (const auto &dep : other.dependencies_) {
                dependencies_.push_back(dep);
            }
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
            return values_.is_valid();
        }

        vector<package_dependency> package::dependencies() const
        {
            return dependencies_;
        }

        int package_config::resolve_package_file(const string &path, const string &filename, ifstream &file)
        {
            char buf[PATH_MAX] = {0};

            snprintf(buf, PATH_MAX, "%s/%s", path.c_str(), filename.c_str());

            if (file_exists(buf)) {
                file.open(buf);

                return PREP_SUCCESS;
            }

            if (filename.find("://") != string::npos) {
                return download_package_file(path, filename, file);
            }

            return PREP_FAILURE;
        }

        int package_config::download_package_file(const string &path, const string &url, ifstream &file)
        {
            string buffer;
            char filename[PATH_MAX + 1] = {0};
            char *fname = NULL;

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
            ifstream file;
            json_object *depjson;
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

            if (!values_.parse(os.str().c_str())) {
                printf("invalid configuration for %s\n", os.str().c_str());
                return PREP_FAILURE;
            }

            if (values_.contains("dependencies")) {
                auto deps = values_.get_array("dependencies");

                for (auto &dep: deps) {
                    dependencies_.push_back(package_dependency(dep));
                }
            }

            if (values_.contains("build_system")) {
                auto bs = values_.get_array("build_system");

                for (auto &command: bs) {
                    build_system_.push_back(command.to_string());
                }
            }

            path_ = build_sys_path(path.c_str(), opts.package_file.c_str(), nullptr);

            log_info("Preparing package \033[1;35m%s\033[0m", name().c_str());

            return PREP_SUCCESS;
        }

        std::string package::name() const
        {
            return values_.get_string("name");
        }

        std::string package::version() const
        {
            return values_.get_string("version");
        }

        vector<string> package::build_system() const
        {
            return build_system_;
        }

        std::string package::location() const
        {
            return values_.get_string("location");
        }

        void package::set_location(const std::string &value)
        {
            values_.set_string("location", value);
        }

        std::string package::build_options() const
        {
            return values_.get_string("build_options");
        }

        std::string package::executable() const
        {
            return values_.get_string("executable");
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
            return values_.contains("name");
        }

        rj::json::object package::get_plugin_config(const plugin *plugin) const
        {
            if (plugin == nullptr) {
                return rj::json::object();
            }

            return values_.get(plugin->name());
        }
    }
}
