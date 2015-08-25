
#include "package_config.h"
#include "log.h"
#include "util.h"
#include <fstream>
#include <sstream>
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#include <libgen.h>

namespace arg3
{
    namespace prep
    {
        static const char *const DEFAULT_LOCATION = ".";

        static const char *const PACKAGE_FILE = "package.json";

        options::options_s() : global(false), package_file(PACKAGE_FILE), location(DEFAULT_LOCATION), force_build(false)
        {}

        package::package() : values_(NULL)
        {
        }

        package::package(json_object *obj) : values_(obj)
        {
            json_object *depjson = NULL;

            if (json_object_object_get_ex(values_, "dependencies", &depjson))
            {
                int len = json_object_array_length(depjson);

                for (int i = 0; i < len; i++)
                {
                    json_object *dep = json_object_array_get_idx(depjson, i);

                    dependencies_.push_back(package_dependency(json_object_get(dep)));
                }
            }

        }

        package::package(const package &other) : values_(json_object_get(other.values_))
        {
            for (const auto &dep : other.dependencies_) {
                dependencies_.push_back(dep);
            }
        }

        package &package::operator=(const package &other) {
            values_ = json_object_get(other.values_);
            for (const auto &dep : other.dependencies_) {
                dependencies_.push_back(dep);
            }
            return *this;
        }

        package::~package()
        {
            if (values_ != NULL)
            {
                json_object_put(values_);
                values_ = NULL;
            }
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

        package_config &package_config::operator=(const package_config &other) {
            package::operator=(other);

            return *this;
        }

        bool package::is_loaded() const
        {
            return values_ != NULL;
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

                return EXIT_SUCCESS;
            }

            if (filename.find("://") != string::npos)
            {
                return download_package_file(path, filename, file);
            }

            return EXIT_FAILURE;
        }

        int package_config::download_package_file(const string &path, const string &url, ifstream &file)
        {

            string buffer;
            char filename [PATH_MAX + 1] = {0};
            char *fname = NULL;

            log_debug("attempting download of %s", url.c_str());

            if (download_to_temp_file(url.c_str(), buffer)) {
                log_error("unable to download %s", url.c_str());
                return EXIT_FAILURE;
            }

            strncpy(filename, url.c_str(), BUFSIZ);

            fname = basename(filename);

            snprintf(filename, PATH_MAX, "%s/%s", path.c_str(), fname);

            if (rename(buffer.c_str(), filename)) {
                log_error("unable to rename %s to %s", buffer.c_str(), filename);
                return EXIT_FAILURE;
            }

            file.open(filename);

            return EXIT_SUCCESS;
        }

        int package_config::load(const std::string &path, const options &opts)
        {
            ifstream file;
            json_object *depjson;
            std::ostringstream os;

            if (path.empty()) {
                return EXIT_FAILURE;
            }

            if (values_ != NULL)
            {
                json_object_put(values_);
                values_ = NULL;
            }

            if (resolve_package_file(path, opts.package_file, file)) {
                log_error("unable to resolve %s", opts.package_file.c_str());
                return EXIT_FAILURE;
            }

            if (!file.is_open())
            {
                log_error("unable to open %s", opts.package_file.c_str());
                return EXIT_FAILURE;
            }

            file >> os.rdbuf();

            values_ = json_tokener_parse(os.str().c_str());

            if (values_ == NULL)
            {
                printf("invalid configuration for %s\n", os.str().c_str());
                return EXIT_FAILURE;
            }

            if (json_object_object_get_ex(values_, "dependencies", &depjson))
            {
                int len = json_object_array_length(depjson);

                for (int i = 0; i < len; i++)
                {
                    json_object *dep = json_object_array_get_idx(depjson, i);

                    dependencies_.push_back(package_dependency(json_object_get(dep)));
                }
            }

            return EXIT_SUCCESS;
        }

        const char *package::name() const
        {
            return get_str("name");
        }

        const char *package::version() const
        {
            return get_str("version");
        }

        const char *package::build_system() const
        {
            return get_str("build_system");
        }

        const char *package::location() const
        {
            return get_str( "location");
        }

        const char *package::build_options() const
        {
            return get_str("build_options");
        }

        void package::set_str(const std::string &key, const std::string &value)
        {
            json_object *obj = NULL;

            if (json_object_object_get_ex(values_, key.c_str(), &obj)) {
                json_object_put(obj);
            }
            json_object_object_add(values_, key.c_str(), json_object_new_string(value.c_str()));
        }

        void package::set_bool(const std::string &key, bool value)
        {
            json_object *obj = NULL;

            if (json_object_object_get_ex(values_, key.c_str(), &obj)) {
                json_object_put(obj);
            }
            json_object_object_add(values_, key.c_str(), json_object_new_boolean(value));
        }

        const char *package::get_str(const std::string &key) const
        {
            json_object *obj = NULL;

            if (json_object_object_get_ex(values_, key.c_str(), &obj)) {
                return json_object_get_string(obj);
            }

            return NULL;
        }

        bool package::get_bool(const std::string &key) const
        {
            json_object *obj = NULL;

            if (json_object_object_get_ex(values_, key.c_str(), &obj)) {
                return json_object_get_boolean(obj);
            }

            return false;
        }
    }
}