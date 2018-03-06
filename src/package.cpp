
#include <sstream>
#include <vector>

#include "common.h"
#include "log.h"
#include "package.h"
#include "plugin.h"
#include "repository.h"
#include "util.h"

namespace micrantha
{
    namespace prep
    {
        Package::Package() = default;

        Package::Package(const json_type &obj) : values_(obj)
        {
            init_dependencies();
            init_build_system();
        }

        void Package::init_dependencies()
        {
            auto value = values_["dependencies"];

            if (!value.is_array()) {
                return;
            }

            for (auto &dep : value) {
                dependencies_.push_back(PackageDependency(dep));
            }
        }

        int Package::dependency_count(const std::string &package_name) const
        {
            int count = 0;

            for (const auto &dep : dependencies()) {
                if (dep.name() == package_name) {
                    count++;
                }
                count += dep.dependency_count(package_name);
            }

            return count;
        }

        void Package::init_build_system()
        {
            if (values_.count("build_system") > 0) {
                auto systems = values_["build_system"];
                for (auto &system : systems) {
                    build_system_.push_back(system);
                }
            }
        }

        Package::Package(const Package &other) = default;

        Package &Package::operator=(const Package &other)
        {
            values_ = other.values_;
            path_ = other.path_;
            dependencies_ = other.dependencies_;
            build_system_ = other.build_system_;
            return *this;
        }

        Package::~Package() = default;

        PackageConfig::PackageConfig() = default;

        PackageConfig::PackageConfig(const PackageConfig &other) = default;

        PackageConfig::~PackageConfig() = default;

        PackageConfig &PackageConfig::operator=(const PackageConfig &other) = default;

        bool Package::is_loaded() const
        {
            return !values_.is_null();
        }

        std::vector<PackageDependency> Package::dependencies() const
        {
            return dependencies_;
        }

        int PackageConfig::resolve_package_file(const std::string &path, const std::string &filename,
                                                std::ifstream &file)
        {
            auto buf = build_sys_path(path, filename);

            if (!file_exists(buf)) {
                return PREP_FAILURE;
            }

            file.open(buf);

            return file.is_open() ? PREP_SUCCESS : PREP_FAILURE;
        }

        int Package::save(const std::string &path) const
        {
            std::ofstream file;

            if (path.empty()) {
                return PREP_FAILURE;
            }

            if (values_.empty()) {
                log::debug("invalid config for ", path);
                return PREP_FAILURE;
            }

            file.open(path);

            if (!file.is_open()) {
                log::debug("unable to open ", path);
                return PREP_FAILURE;
            }

            file << values_.dump();

            return PREP_SUCCESS;
        }

        int PackageConfig::load(const std::string &path, const Options &opts)
        {
            std::ifstream file;
            std::ostringstream os;

            if (path.empty()) {
                return PREP_FAILURE;
            }

            if (resolve_package_file(path, opts.package_file, file) != PREP_SUCCESS) {
                log::debug("unable to resolve ", path, "/", opts.package_file);
                return PREP_FAILURE;
            }

            if (!file.is_open()) {
                log::error("unable to open ", opts.package_file);
                return PREP_FAILURE;
            }

            file >> os.rdbuf();

            values_ = json_type::parse(os.str().c_str());

            if (values_.empty()) {
                log::error("invalid configuration for ", os.str());
                return PREP_FAILURE;
            }

            init_dependencies();

            init_build_system();

            path_ = build_sys_path(path, opts.package_file);

            return PREP_SUCCESS;
        }

        std::string Package::name() const
        {
            auto name = values_.find("name");
            if (name == values_.end()) {
                return "";
            }
            return *name;
        }

        std::string Package::version() const
        {
            auto value = values_.find("version");
            if (value == values_.end()) {
                return "";
            }
            return *value;
        }

        std::vector<std::string> Package::build_system() const
        {
            return build_system_;
        }

        std::string Package::location() const
        {
            auto loc = values_.find("location");
            if (loc == values_.end()) {
                return "";
            }
            return *loc;
        }

        std::string Package::build_options() const
        {
            auto value = values_.find("build_options");
            if (value == values_.end()) {
                return "";
            }
            return *value;
        }

        std::string Package::executable() const
        {
            return values_["executable"];
        }

        std::string Package::path() const
        {
            return path_;
        }

        bool Package::has_path() const
        {
            return !path_.empty();
        }

        bool Package::has_name() const
        {
            return values_.count("name") > 0;
        }

        Package::json_type Package::get_value(const std::string &key) const
        {
            auto config = values_.find(key);

            if (config == values_.end()) {
                return json_type();
            }
            return *config;
        }

        PackageDependency::PackageDependency(const Package::json_type &obj) : Package(obj)
        {
        }

        PackageDependency::PackageDependency(const PackageDependency &other) = default;

        PackageDependency::~PackageDependency() = default;

        int PackageDependency::load(const std::string &path, const Options &opts)
        {
            return PREP_SUCCESS;
        }
    }
}
