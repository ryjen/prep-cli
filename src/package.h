#ifndef MICRANTHA_PREP_PACKAGE_CONFIG_H
#define MICRANTHA_PREP_PACKAGE_CONFIG_H

#include <fstream>
#include <string>
#include <experimental/optional>
#include <vector>
#include "json.hpp"
#include "options.h"

namespace micrantha
{
    namespace prep
    {
        // forward declarations
        class PackageDependency;
        class Plugin;

        /**
         * a abstract package
         */
        class Package
        {
           public:
            typedef nlohmann::json json_type;
            virtual ~Package();

            /* properties */
            std::string name() const;
            std::string version() const;
            std::vector<std::string> build_system() const;
            std::string build_options() const;
            std::string path() const;
            virtual std::string location() const;
            std::vector<PackageDependency> dependencies() const;
            std::string executable() const;

            /* validators */
            bool has_path() const;
            bool has_name() const;
            bool is_loaded() const;

            /**
             * loads a package
             * @param path the path to the package
             * @opts options specified from command line
             * @return PREP_SUCCESS if loaded, otherwise PREP_FAILURE
             */
            virtual int load(const std::string &path, const Options &opts) = 0;

            int save(const std::string &path) const;

            /**
             * gets the plugin specific configuration defined the package configuration
             * @param plugin the plugin to get configuration for
             * @return the JSON configuration
             */
            json_type get_value(const std::string &key) const;

            /**
             * counts the number of dependencies for a package in the configuration
             * @package_name the name of the dependency
             * @return the number of dependencies
             */
            int dependency_count(const std::string &package_name) const;

            std::experimental::optional<PackageDependency> find_dependency(const std::string &package_name) const;

           protected:
            /* constructors */
            Package();
            explicit Package(const json_type &obj);
            Package(const Package &other);
            Package &operator=(const Package &other);

            /* initializers */
            void init_dependencies();
            void init_build_system();

            json_type values_;
            std::string path_;
            std::vector<PackageDependency> dependencies_;
            std::vector<std::string> build_system_;
            friend class Plugin;
        };

        /**
         * configuration for a package dependency
         */
        class PackageDependency : public Package
        {
            friend class Package;
            friend class PackageConfig;

           public:
            ~PackageDependency() override;
            PackageDependency(const PackageDependency &other);
            PackageDependency &operator=(const PackageDependency &) = default;

            int load(const std::string &path, const Options &opts) override;

           protected:
            explicit PackageDependency(const json_type &obj);
        };

        /**
         * configuration for a package
         */
        class PackageConfig : public Package
        {
           public:
            PackageConfig();
            PackageConfig(const PackageConfig &);
            ~PackageConfig() override;
            PackageConfig &operator=(const PackageConfig &other);

            /**
             * override for packages
             */
            int load(const std::string &path, const Options &opts) override;

           private:
            int resolve_package_file(const std::string &path, const std::string &filename, std::ifstream &file);
        };
    }
}

#endif
