#include "package_builder.h"
#include "package_resolver.h"
#include "util.h"
#include "log.h"
#include <cerrno>
#include <cassert>
#include <pwd.h>
#include <uuid/uuid.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>

namespace std {

    istream& operator >> (istream& is, pair<string, string>& ps)
    {
        return is >> ps.first >> ps.second;
    }
    ostream& operator << (ostream& os, const pair<const string, string>& ps)
    {
        return os << ps.first << " " << ps.second;
    }
}

namespace arg3
{
    namespace prep
    {

        const char *const package_builder::HISTORY_FILE = ".history";

        const char *const package_builder::LOCAL_REPO = ".prep";

        const char *const package_builder::GLOBAL_REPO = "/usr/local/share/prep";

        const char *const package_builder::META_FOLDER = ".meta";

        int package_builder::initialize(const options &opts)
        {
            char path[BUFSIZ + 1] = {0};

            if (opts.global)
            {
                repo_path_ = GLOBAL_REPO;
            }
            else
            {
                repo_path_ = string(get_home_dir()) + "/" + LOCAL_REPO;
            }


            if (!directory_exists(repo_path_.c_str())) {
                int ch;

                printf("OK to create folder %s? (Y/n) ", repo_path_.c_str());

                ch = getchar();

                if (ch != 10 && toupper(ch) != 'Y') {
                    return EXIT_FAILURE;
                }

                mkdir(repo_path_.c_str(), 0700);
            }

            strncpy(path, getenv("PATH"), BUFSIZ);

            char *dir = strtok(path, ":");

            while (dir != NULL)
            {
                if (repo_path_ == dir) {
                    break;
                }

                dir = strtok(NULL, ":");
            }

            if (dir == NULL)
            {
                std::cout << "Warning: " << repo_path_ << " is not added to your PATH\n";
            }

            return EXIT_SUCCESS;
        }

        int package_builder::build_package(const package &config, const char *path)
        {
            if (!str_cmp(config.build_system(), "autotools"))
            {
                if ( build_autotools(config, path) ) {
                    return EXIT_FAILURE;
                }
            }
            else if (!str_cmp(config.build_system(), "cmake"))
            {
                if ( build_cmake(config, path) ) {
                    return EXIT_FAILURE;
                }
            }
            else if (!str_cmp(config.build_system(), "make"))
            {
                if ( build_make(config, path)) {
                    return EXIT_FAILURE;
                }
            }
            else
            {
                log_error("unknown build system '%s'", config.build_system());
                return EXIT_FAILURE;
            }

            if (save_meta(config)) {
                log_error("unable to save meta data");
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }

        int package_builder::save_meta(const package &config) const
        {
            if (config.version() == NULL) {
                return EXIT_SUCCESS;
            }

            string metaDir = repo_path_ + "/" + META_FOLDER;

            if (!directory_exists(metaDir.c_str())) {
                if (mkpath(metaDir.c_str(), 0777)) {
                    log_errno(errno);
                    return EXIT_FAILURE;
                }
            }

            ofstream out(metaDir + "/" + config.name());

            out << config.version() << endl;

            out.close();

            return EXIT_SUCCESS;
        }

        int package_builder::build(const package &config, options &opts, const std::string &path)
        {
            assert(config.is_loaded());

            if (config.location() != NULL) {
                save_history(config.location(), path);
            }

            for (package_dependency &p : config.dependencies())
            {
                package_resolver resolver;
                string working_dir;

                log_info("Building dependency %s...", p.name());

                working_dir = exists_in_history(p.location());

                if (working_dir.empty() || !directory_exists(working_dir.c_str())) {

                    if (resolver.resolve_package(p, opts, p.location())) {
                        return EXIT_FAILURE;
                    }

                    working_dir = resolver.working_dir();

                }

                if (build(p, opts, working_dir)) {
                    return EXIT_FAILURE;
                }
            }

            if ( build_package(config, path.c_str()) ) {
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }

        std::string package_builder::exists_in_history(const std::string &location) const
        {
            ifstream is(repo_path_ + "/" + HISTORY_FILE);

            std::map<std::string, std::string> mps;
            std::insert_iterator< std::map<std::string, std::string> > mpsi(mps, mps.begin());

            const std::istream_iterator<std::pair<std::string, std::string> > eos;
            std::istream_iterator<std::pair<std::string, std::string> > its (is);

            std::copy(its, eos, mpsi);

            is.close();

            return mps[location];
        }

        void package_builder::save_history(const std::string &location, const std::string &working_dir) const
        {
            ifstream is(repo_path_ + "/" + HISTORY_FILE);

            std::map<std::string, std::string> mps;
            std::insert_iterator< std::map<std::string, std::string> > mpsi(mps, mps.begin());

            const std::istream_iterator<std::pair<std::string, std::string> > eos;
            std::istream_iterator<std::pair<std::string, std::string> > its (is);

            std::copy(its, eos, mpsi);

            is.close();

            mps[location] = working_dir;

            ofstream os(repo_path_ + "/" + HISTORY_FILE);

            std::copy(mps.begin(), mps.end(), std::ostream_iterator<std::pair<std::string, std::string> >(os, "\n"));

            os.close();
        }

        const char *const package_builder::get_home_dir() const
        {
            char *homedir = getenv("HOME");

            if (homedir == NULL)
            {
                uid_t uid = getuid();
                struct passwd *pw = getpwuid(uid);

                if (pw == NULL)
                {
                    log_error("Failed to get user home directory");
                    exit(EXIT_FAILURE);
                }

                homedir = pw->pw_dir;
            }

            return homedir;
        }

        string package_builder::build_ldflags(const package &config, const string &varName) const
        {
            ostringstream buf;
            char *temp;

            if (directory_exists(LOCAL_REPO)) {
                buf << "-L " << LOCAL_REPO << "/lib ";
            }

            if (directory_exists(GLOBAL_REPO)) {
                buf << "-L " << GLOBAL_REPO << "/lib ";
            }

            temp = getenv(varName.c_str());

            if (temp) {
                buf << temp;
            }

            string flags = buf.str();

            if (flags.empty()) {
                return flags;
            }

            return varName + "=\"" + flags + "\"";
        }

        string package_builder::build_cflags(const package &config, const string &varName) const
        {
            ostringstream buf;
            char *temp;

            if (directory_exists(LOCAL_REPO)) {
                buf << "-I " << LOCAL_REPO << "/include ";
            }

            if (directory_exists(GLOBAL_REPO)) {
                buf << "-I " << GLOBAL_REPO << "/include ";
            }

            temp = getenv(varName.c_str());

            if (temp) {
                buf << temp;
            }

            string flags = buf.str();

            if (flags.empty()) {
                return flags;
            }

            return varName + "=\"" + flags + "\"";
        }

        int package_builder::build_cmake(const package &config, const char *path)
        {
            char buf[BUFSIZ + 1] = {0};
            char flags[5][BUFSIZ];

            log_debug("building cmake [%s]", path);

            snprintf(buf, BUFSIZ, "cmake -DCMAKE_INSTALL_PREFIX:PATH=%s .", repo_path_.c_str());

            const char *cmake_args[] = { "/bin/sh", "-c", buf, NULL };

            strncpy(flags[0], build_cflags(config, "CPPFLAGS").c_str(), BUFSIZ);
            strncpy(flags[1], build_cflags(config, "CXXFLAGS").c_str(), BUFSIZ);
            strncpy(flags[2], build_cflags(config, "CFLAGS").c_str(), BUFSIZ);
            strncpy(flags[3], build_ldflags(config, "LDFLAGS").c_str(), BUFSIZ);
            snprintf(flags[4], BUFSIZ, "PATH=%s", getenv("PATH"));

            char *const envp[] = {
                flags[0], flags[1], flags[2], flags[3], flags[4], NULL
            };

            if (fork_command(cmake_args, path, envp))
            {
                log_error("unable to execute cmake");
                return EXIT_FAILURE;
            }

            return build_make(config, path);
        }

        int package_builder::build_make(const package &config, const char *path)
        {
            const char *make_args[] = { "/bin/sh", "-c", "make install", NULL };

            if (fork_command(make_args, path, NULL))
            {
                log_error("unable to execute make");
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }

        int package_builder::build_autotools(const package &config, const char *path)
        {
            char buf[BUFSIZ + 1] = {0};

            char flags[5][BUFSIZ];

            log_debug("building autotools [%s]", path);

            snprintf(buf, BUFSIZ, "%s/configure", path);

            if (file_exists(buf)) {
                snprintf(buf, BUFSIZ, "./configure --prefix=%s", repo_path_.c_str());
            } else {

                snprintf(buf, BUFSIZ, "%s/autogen.sh", path);

                if (!file_exists(buf)) {
                    log_error("Don't know how to build %s... no autotools configuration found.", config.name());
                    return EXIT_FAILURE;
                }

                snprintf(buf, BUFSIZ, "./autogen.sh --prefix=%s", repo_path_.c_str());
            }

            const char *configure_args[] = { "/bin/sh", "-c", buf, NULL };

            strncpy(flags[0], build_cflags(config, "CPPFLAGS").c_str(), BUFSIZ);
            strncpy(flags[1], build_cflags(config, "CXXFLAGS").c_str(), BUFSIZ);
            strncpy(flags[2], build_cflags(config, "CFLAGS").c_str(), BUFSIZ);
            strncpy(flags[3], build_ldflags(config, "LDFLAGS").c_str(), BUFSIZ);
            snprintf(flags[4], BUFSIZ, "PATH=%s", getenv("PATH"));

            char *const envp[] = {
                flags[0], flags[1], flags[2], flags[3], flags[4], NULL
            };

            if (fork_command(configure_args, path, envp))
            {
                log_error("unable to execute configure");
                return EXIT_FAILURE;
            }

            return build_make(config, path);
        }

        int package_builder::build_from_folder(options &opts, const char *path)
        {
            package_config config;

            if (config.load(path, opts))
            {
                return build(config, opts, path);
            }
            else
            {
                log_error("%s is not a valid prep package\n", path);
                return 1;
            }
        }
    }
}