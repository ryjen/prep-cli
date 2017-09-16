
#include <sstream>
#include <vector>

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
        package::package() = default;

        package::package(const json_type &obj) : values_(obj)
        {
            init_dependencies();
            init_build_system();
        }

        void package::init_dependencies()
        {
            auto value = values_["dependencies"];

            if (!value.is_array()) {
                return;
            }

            for (auto &dep : value) {
                dependencies_.push_back(package_dependency(dep));
            }
        }


        int package::dependency_count(const std::string &package_name) const
        {
            int count = 0;

            for (const package_dependency &dep : dependencies()) {
                if (strcmp(dep.name().c_str(), package_name.c_str()) == 0) {
                    count++;
                }
                count += dep.dependency_count(package_name);
            }

            return count;
        }


        void package::init_build_system()
        {
            if (values_.count("build_system") > 0) {
                std::vector<std::string> systems = values_["build_system"];
                for (auto &system : systems) {
                    build_system_.push_back(system);
                }
            }
        }

        package::package(const package &other) = default;

        package &package::operator=(const package &other)
        {
            values_       = other.values_;
            path_         = other.path_;
            dependencies_ = other.dependencies_;
            build_system_ = other.build_system_;
            return *this;
        }

        package::~package() = default;

        package_config::package_config() = default;

        package_config::package_config(const package_config &other) = default;

        package_config::~package_config() = default;

        package_config &package_config::operator=(const package_config &other)= default;

        bool package::is_loaded() const
        {
            return !values_.is_null();
        }

        std::vector<package_dependency> package::dependencies() const
        {
            return dependencies_;
        }

        int package_config::resolve_package_file(const std::string &path, const std::string &filename,
                                                 std::ifstream &file) {
            auto buf = build_sys_path(path.c_str(), filename.c_str(), NULL);


            if (!file_exists(buf)) {
                return PREP_FAILURE;
            }

            file.open(buf);

            return file.is_open() ? PREP_SUCCESS : PREP_FAILURE;
        }

        int package_config::load(const std::string &path, const options &opts)
        {
            std::ifstream file;
            std::ostringstream os;

            if (path.empty()) {
                return PREP_FAILURE;
            }

            if (resolve_package_file(path, opts.package_file, file) != PREP_SUCCESS) {
                log_debug("unable to resolve %s/%s", path.c_str(), opts.package_file.c_str());
                return PREP_FAILURE;
            }

            if (!file.is_open()) {
                log_error("unable to open %s", opts.package_file.c_str());
                return PREP_FAILURE;
            }

            file >> os.rdbuf();

            values_ = json_type::parse(os.str().c_str());

            if (values_.empty()) {
                log_error("invalid configuration for %s\n", os.str().c_str());
                return PREP_FAILURE;
            }

            init_dependencies();

            init_build_system();

            path_ = build_sys_path(path.c_str(), opts.package_file.c_str(), NULL);

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
            auto value = values_.find("version");
            if (value == values_.end()) {
                return "";
            }
            return *value;
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
            auto value =  values_.find("build_options");
            if (value == values_.end()) {
                return "";
            }
            return *value;
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
            return values_.count("name") > 0;
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

        package_dependency::package_dependency(const package_dependency &other) = default;

        package_dependency::~package_dependency() = default;

        int package_dependency::load(const std::string &path, const options &opts)
        {
            return PREP_SUCCESS;
        }
    }
}
