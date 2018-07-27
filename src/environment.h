#ifndef MICRANTHA_PREP_ENVIRONMENT_H
#define MICRANTHA_PREP_ENVIRONMENT_H

#include <string>
#include <vector>
#include <map>

namespace micrantha
{
    namespace prep
    {
        namespace environment
        {
            /**
             * gets an environment variable
             * @param key the key of the variable
             * @return the value of key
             */
            std::string get(const std::string &key);

            /**
             * sets an environment variable
             * @param key the key of the value
             * @param value the value
             * @param overwrite true if an existing key should be overwritten
             */
            void set(const std::string &key, const std::string &value, bool overwrite = true);

            /**
             * creates CFLAGS for building using specified var name (CXXFLAGS, CPPFLAGS, CFLAGS, etc)
             * @param varName the name of the variable to append preps cflags with
             * @return the build flags with preps build flags
             */
            std::string build_cflags(const std::string &varName);

            /**
             * creates LDFLAGS for linking using the specified var name
             * NOTE: paths are based on the current repository if exists
             * @param varName the name of the variable to append preps link flags with
             * @return the link flags with preps link flags
             */
            std::string build_ldflags(const std::string &varName);

            /**
             * creates the bin path with preps bin path
             * NOTE: paths are based on the current repository if exists
             * @return a colon separated path string
             */
            std::string build_path(const std::string &varName);

            /**
             * creates the link path with preps link path
             * NOTE: paths are based on the current repository if exists
             * @return a colon separated path string
             */
            std::string build_ldpath(const std::string &varName);

            /**
             * creates a map of build variable keys and values (build, link, and paths)
             * NOTE: paths are based on the current repository if exists
             * @return a map of environment variables
             */
            std::map<std::string,std::string> build_map();

            /**
             * creates a map of runtime variable keys and values
             * NOTE: paths are based on the current repository if exists
             * @return a map of environment variables
             */
            std::map<std::string,std::string> run_map();

            /**
             * creates a list of build environment variables
             * NOTE: paths are based on the current repository if exists
             * @return list of key=value strings
             */
            std::vector<std::string> build_env();

            /**
             * creates a list of runtime environment variables
             * NOTE: paths are based on the current repository if exists
             * @return a list of key=value strings
             */
            std::vector<std::string> run_env();
        }
    }
}

#endif
