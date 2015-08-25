#include "package_builder.h"
#include "package_resolver.h"
#include "util.h"
#include "log.h"
#include <cerrno>
#include <cassert>
#include <pwd.h>
#include <dirent.h>
#include <fts.h>
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

        const char *const package_builder::INSTALL_FOLDER = ".install";

        int package_builder::initialize(const options &opts)
        {
            char path[BUFSIZ + 1] = {0};

            if (opts.global)
            {
                repo_path_ = GLOBAL_REPO;
            }
            else
            {
                repo_path_ = get_home_dir();
            }


            force_build_ = opts.force_build;

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

        string package_builder::get_home_dir() const
        {
            static string home_dir_path;

            if (home_dir_path.empty())
            {
                char * homedir = getenv("HOME");

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

                home_dir_path = homedir;

                home_dir_path += "/";
                home_dir_path += LOCAL_REPO;
            }

            return home_dir_path;
        }

        string package_builder::get_install_dir(const package &config) const
        {
            if (config.name() == NULL) {
                return "/tmp/unknown";
            }

            return repo_path_ + "/" + INSTALL_FOLDER + "/" + config.name();
        }

        int package_builder::build_package(const package & config, const char *path)
        {
            string installPath;

            if (!force_build_ && has_meta(config) == EXIT_SUCCESS) {
                return EXIT_SUCCESS;
            }

            installPath = get_install_dir(config);

            log_trace("Installing to %s...", installPath.c_str());

            if (!directory_exists(installPath.c_str())) {
                log_trace("Creating %s...", installPath.c_str());
                if (mkpath(installPath.c_str(), 0777)) {
                    log_errno(errno);
                    return EXIT_FAILURE;
                }
            }

            if (!str_cmp(config.build_system(), "autotools"))
            {
                if ( build_autotools(config, path, installPath.c_str()) ) {
                    return EXIT_FAILURE;
                }
            }
            else if (!str_cmp(config.build_system(), "cmake"))
            {
                if ( build_cmake(config, path, installPath.c_str()) ) {
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

        int package_builder::save_meta(const package & config) const
        {
            string metaDir = repo_path_ + "/" + META_FOLDER;

            if (!directory_exists(metaDir.c_str())) {
                if (mkpath(metaDir.c_str(), 0777)) {
                    log_errno(errno);
                    return EXIT_FAILURE;
                }
            }

            ofstream out(metaDir + "/" + config.name());

            if (config.version()) {
                out << config.version() << endl;
            } else if (config.location()) {
                out << config.location() << endl;
            }

            out.close();

            return EXIT_SUCCESS;
        }

        int package_builder::has_meta(const package & config) const
        {
            string metaDir = repo_path_ + "/" + META_FOLDER;

            if (!directory_exists(metaDir.c_str())) {
                return EXIT_FAILURE;
            }

            ifstream in(metaDir + "/" + config.name());

            string info;
            in >> info;
            in.close();

            if (config.version()) {
                if (strcmp(info.c_str(), config.version()) >= 0) {
                    log_info("Already have %s version %s", config.name(), info.c_str());
                    return EXIT_SUCCESS;
                }
            }
            else if (config.location()) {
                if (!strcmp(info.c_str(), config.location())) {
                    log_info("Already have %s from %s", config.name(), info.c_str());
                    return EXIT_SUCCESS;
                }
            }

            return EXIT_FAILURE;
        }

        /*
        int package_builder::link(const package & config, const string & fromPath, const string & toPath)
        {
            string nextPath;
            struct dirent *d_ent = NULL;

            DIR *dir = opendir(fromPath.c_str());

            if (dir == NULL) {
                log_errno(errno);
                return EXIT_FAILURE;
            }

            while ((d_ent = readdir(dir))) {
                struct stat st;
                string fullFromPath;
                string fullToPath;

                if (*d_ent->d_name == '.') {
                    continue;
                }

                fullFromPath = fromPath + "/" + d_ent->d_name;

                fullToPath = toPath + "/" + d_ent->d_name;

                if (stat(fullFromPath.c_str(), &st)) {
                    log_errno(errno);
                    continue;
                }

                if (S_ISDIR(st.st_mode)) {

                    if (mkpath(fullToPath.c_str(), 0777)) {
                        log_errno(errno);
                        closedir(dir);
                        return EXIT_FAILURE;
                    }

                    if (link(config, fullFromPath, fullToPath)) {
                        log_error("unable to link directory contents %s to %s", fullFromPath.c_str(), fullToPath.c_str());
                        closedir(dir);
                        return EXIT_FAILURE;
                    }
                }

                else if (S_ISREG(st.st_mode)) {
                    if (symlink(fullFromPath.c_str(), fullToPath.c_str())) {
                        log_error("unable to link %s to %s", fullFromPath.c_str(), fullToPath.c_str());
                        closedir(dir);
                        return EXIT_FAILURE;
                    }

                    log_trace("Linked %s to %s", fullFromPath.c_str(), fullToPath.c_str());
                }
            }

            return EXIT_SUCCESS;
        }*/

        int package_builder::build(const package & config, options & opts, const std::string & path)
        {
            string installDir;

            assert(config.is_loaded());

            log_trace("Building from %s...", path.c_str());

            if (config.location() != NULL) {
                save_history(config.location(), path);
            }

            installDir = get_install_dir(config);

            if (!directory_exists(installDir.c_str())) {
                if (mkpath(installDir.c_str(), 0777)) {
                    log_error("could not create [%s] (%s)", installDir.c_str(), strerror(errno));
                    return EXIT_FAILURE;
                }
            }

            for (package_dependency &p : config.dependencies())
            {
                package_resolver resolver;
                string working_dir;

                if (has_meta(p) == EXIT_SUCCESS) {
                    continue;
                }

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

            if ( link_package(config) ) {
                log_error("Unable to link package");
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }

        int package_builder::link_package(const package & config) const
        {
            FTS *file_system = NULL;
            FTSENT *child = NULL;
            FTSENT *parent = NULL;
            int rval = EXIT_SUCCESS;
            char buf[PATH_MAX + 1] = {0};
            struct stat st;

            char *const paths[] = {
                (char *const) get_install_dir(config).c_str(),
                NULL
            };

            file_system = fts_open(paths, FTS_COMFOLLOW | FTS_NOCHDIR, NULL);

            if (file_system == NULL) {
                log_error("unable to open file system [%s]", strerror(errno));
                return EXIT_FAILURE;
            }

            size_t plength = get_install_dir(config).length();

            while ( (parent = fts_read(file_system)) != NULL)
            {
                if (strlen(parent->fts_path) <= plength) {
                    continue;
                }

                // get the repo path
                snprintf(buf, PATH_MAX, "%s%s", repo_path_.c_str(), parent->fts_path + plength);

                // and check if it already exists
                if (stat(buf, &st) == 0) {
                    log_debug("%s exists, skipping", buf, strerror(errno));
                    continue;
                }

                // check the install file is a directory
                if (stat(parent->fts_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    // create the repo path directory
                    if (mkdir(buf, 0777)) {
                        log_error("Could not create %s [%s]", buf, strerror(errno));
                        rval = EXIT_FAILURE;
                        break;
                    }
                    continue;
                }

                log_debug("linking [%s] to [%s]", parent->fts_path, buf);

                if ( symlink(parent->fts_path, buf) ) {
                    log_error("unable to link [%s]", strerror(errno));
                    rval = EXIT_FAILURE;
                    break;
                }
            }

            fts_close(file_system);

            return rval;
        }

        int package_builder::unlink_package(const package & config) const
        {
            FTS *file_system = NULL;
            FTSENT *child = NULL;
            FTSENT *parent = NULL;
            int rval = EXIT_SUCCESS;
            char buf[PATH_MAX + 1] = {0};
            struct stat st;

            char *const paths[] = {
                (char *const) get_install_dir(config).c_str(),
                NULL
            };

            file_system = fts_open(paths, FTS_COMFOLLOW | FTS_NOCHDIR, NULL);

            if (file_system == NULL) {
                log_error("unable to open file system [%s]", strerror(errno));
                return EXIT_FAILURE;
            }

            size_t plength = get_install_dir(config).length();

            while ( (parent = fts_read(file_system)) != NULL)
            {
                if (strlen(parent->fts_path) <= plength) {
                    continue;
                }

                // get the repo path
                snprintf(buf, PATH_MAX, "%s%s", repo_path_.c_str(), parent->fts_path + plength);

                if (stat(buf, &st)) {
                    log_debug("%s not found, skipping", buf, strerror(errno));
                    continue;
                }

                if (!S_ISLNK(st.st_mode)) {
                    log_debug("%s is not a link, skipping", buf);
                    continue;
                }

                log_debug("unlinking [%s]", buf);

                if ( unlink(buf) ) {
                    log_error("unable to unlink [%s]", strerror(errno));
                    rval = EXIT_FAILURE;
                    break;
                }
            }

            fts_close(file_system);

            return rval;
        }

        std::string package_builder::exists_in_history(const std::string & location) const
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

        void package_builder::save_history(const std::string & location, const std::string & working_dir) const
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

        string package_builder::build_ldflags(const package & config, const string & varName) const
        {
            ostringstream buf;
            char *temp;

            if (directory_exists(get_home_dir().c_str())) {
                buf << "-L" << get_home_dir() << "/lib ";
            }

            if (directory_exists(GLOBAL_REPO)) {
                buf << "-L" << GLOBAL_REPO << "/lib ";
            }

            temp = getenv(varName.c_str());

            if (temp) {
                buf << temp;
            }

            string flags = buf.str();

            if (flags.empty()) {
                return flags;
            }

            return varName + "=" + flags;
        }

        string package_builder::build_cflags(const package & config, const string & varName) const
        {
            ostringstream buf;
            char *temp;

            if (directory_exists(get_home_dir().c_str())) {
                buf << "-I " << get_home_dir() << "/include ";
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

            return varName + "=" + flags;
        }

        string package_builder::build_path(const package & config) const
        {
            ostringstream buf;
            char *temp;

            if (directory_exists(get_home_dir().c_str())) {
                buf << get_home_dir() << "/bin:";
            }

            if (directory_exists(GLOBAL_REPO)) {
                buf << GLOBAL_REPO << "/bin:";
            }

            temp = getenv("PATH");

            if (temp) {
                buf << temp;
            }

            string flags = buf.str();

            if (flags.empty()) {
                return flags;
            }

            return "PATH=" + flags;
        }

        int package_builder::build_cmake(const package & config, const char *path, const char *toPath)
        {
            char buf[BUFSIZ + 1] = {0};
            char flags[5][BUFSIZ];
            const char *buildopts = NULL;

            log_debug("building cmake [%s]", path);

            buildopts = config.build_options();

            if (buildopts == NULL) {
                buildopts = "";
            }

            snprintf(buf, BUFSIZ, "cmake -DCMAKE_INSTALL_PREFIX:PATH=%s %s .", get_install_dir(config).c_str(), buildopts);

            const char *cmake_args[] = { "/bin/sh", "-c", buf, NULL };

            strncpy(flags[0], build_cflags(config, "CPPFLAGS").c_str(), BUFSIZ);
            strncpy(flags[1], build_cflags(config, "CXXFLAGS").c_str(), BUFSIZ);
            strncpy(flags[3], build_ldflags(config, "LDFLAGS").c_str(), BUFSIZ);
            strncpy(flags[4], build_path(config).c_str(), BUFSIZ);

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

        int package_builder::build_make(const package & config, const char *path)
        {
            const char *make_args[] = { "/bin/sh", "-c", "make install", NULL };

            if (fork_command(make_args, path, NULL))
            {
                log_error("unable to execute make");
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }

        int package_builder::build_autotools(const package & config, const char *path, const char *toPath)
        {
            char buf[BUFSIZ + 1] = {0};
            char flags[5][BUFSIZ];
            const char *buildopts = NULL;

            log_debug("building autotools [%s]", path);

            buildopts = config.build_options();

            if (buildopts == NULL) {
                buildopts = "";
            }

            snprintf(buf, BUFSIZ, "%s/configure", path);

            if (file_exists(buf)) {
                snprintf(buf, BUFSIZ, "./configure --prefix=%s %s", get_install_dir(config).c_str(), buildopts);
            } else {

                snprintf(buf, BUFSIZ, "%s/autogen.sh", path);

                if (!file_exists(buf)) {
                    log_error("Don't know how to build %s... no autotools configuration found.", config.name());
                    return EXIT_FAILURE;
                }

                snprintf(buf, BUFSIZ, "./autogen.sh --prefix=%s %s", get_install_dir(config).c_str(), buildopts);
            }

            const char *configure_args[] = { "/bin/sh", "-c", buf, NULL };

            strncpy(flags[0], build_cflags(config, "CPPFLAGS").c_str(), BUFSIZ);
            strncpy(flags[1], build_cflags(config, "CXXFLAGS").c_str(), BUFSIZ);
            strncpy(flags[3], build_ldflags(config, "LDFLAGS").c_str(), BUFSIZ);
            strncpy(flags[4], build_path(config).c_str(), BUFSIZ);

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

        int package_builder::build_from_folder(options & opts, const char *path)
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