
#include <signal.h>
#include <unistd.h>
#include <util.h>
#include <fstream>
#include <sstream>
#include <thread>

#include "common.h"
#include "environment.h"
#include "log.h"
#include "package_config.h"
#include "plugin.h"
#include "util.h"

namespace micrantha
{
    namespace prep
    {
        const unsigned char default_plugins_zip[] = {
#include "plugins.inc"
        };

        const unsigned int default_plugins_size = sizeof(default_plugins_zip) / sizeof(default_plugins_zip[0]);

        namespace helper
        {
            constexpr static const char *const HOOK_NAMES[] = {"load",   "unload",  "install",
                                                               "remove", "resolve", "build"};

            constexpr static const char *const TYPE_INTERNAL = "internal";

            // test a plugin path for a manifest file
            bool is_valid_plugin_path(const std::string &path)
            {
                if (path.empty()) {
                    return false;
                }

                auto manifest = build_sys_path(path.c_str(), Plugin::MANIFEST_FILE, NULL);

                return file_exists(manifest);
            }

            // test a plugin path to see if its type is internal
            bool is_plugin_internal(const std::string &path)
            {
                if (path.empty()) {
                    return false;
                }

                auto manifest = build_sys_path(path.c_str(), Plugin::MANIFEST_FILE, NULL);

                std::ifstream file(manifest);

                if (!file.is_open()) {
                    return false;
                }

                std::ostringstream buf;
                file >> buf.rdbuf();
                auto config = Package::json_type::parse(buf.str().c_str());

                auto type = config["type"];

                return type.is_string() && type.get<std::string>() == TYPE_INTERNAL;
            }

            // test input for a return command
            bool is_return_command(const std::string &line)
            {
                return line.length() > 7 && !strcasecmp(line.substr(0, 7).c_str(), "RETURN ");
            }

            // writes a line to a file descriptor
            ssize_t write_line(int fd, const std::string &line)
            {
                ssize_t n1 = write(fd, line.c_str(), line.length());

                if (n1 < 0) {
                    return n1;
                }

                auto n2 = write(fd, "\n", 1);

                if (n2 < 0) {
                    return n2;
                }

                return n1 + n2;
            }

            // writes a header to the parent process, resulting as input to child process
            int write_header(int master, const std::string &method, const std::vector<std::string> &info)
            {
                // write the hook method to the plugin (child)
                if (write_line(master, method) < 0) {
                    return PREP_FAILURE;
                }

                // for each additional argument...
                for (auto &i : info) {
                    // write to the child
                    if (write_line(master, i) < 0) {
                        return PREP_FAILURE;
                    }
                }

                // write the header terminator
                if (write_line(master, "END") < 0) {
                    return PREP_FAILURE;
                }

                return PREP_SUCCESS;
            }

            // reads a line from a file descriptor
            ssize_t read_line(int fd, std::string &buf)
            {
                ssize_t numRead; /* # of bytes fetched by last read() */
                size_t totRead;  /* Total bytes read so far */
                char ch;

                totRead = 0;

                buf.clear();

                for (;;) {
                    numRead = read(fd, &ch, 1);

                    if (numRead == -1) {
                        if (errno == EINTR) /* Interrupted --> restart read() */
                            continue;
                        else
                            return -1; /* Some other error */

                    } else if (numRead == 0) { /* EOF */
                        if (totRead == 0)      /* No bytes read; return 0 */
                            return 0;
                        else /* Some bytes read; add '\0' */
                            break;

                    } else { /* 'numRead' must be 1 if we get here */
                        totRead++;

                        if (ch != '\n' && ch != '\r') {
                            buf += ch;
                        }

                        if (ch == '\n')
                            break;
                    }
                }

                return totRead;
            }
        }

        Plugin::Plugin(const std::string &name) : name_(name)
        {
        }

        Plugin::~Plugin()
        {
            on_unload();
        }

        bool Plugin::is_verbose() const
        {
            return verbose_;
        }

        Plugin &Plugin::set_verbose(bool value)
        {
            verbose_ = value;
            return *this;
        }

        int Plugin::load(const std::string &path)
        {
            basePath_ = path;

            std::string manifest = build_sys_path(path.c_str(), MANIFEST_FILE, NULL);

            std::ifstream file(manifest);

            if (!file.is_open()) {
                return PREP_ERROR;
            }

            std::ostringstream buf;

            file >> buf.rdbuf();

            auto config = Package::json_type::parse(buf.str().c_str());

            if (config.size() == 0) {
                log_error("invalid configuration for plugin [%s]", name_.c_str());
                return PREP_FAILURE;
            }

            auto entry = config["type"];

            if (entry.is_string()) {
                type_ = entry.get<std::string>();
            }

            entry = config["version"];

            if (entry.is_string()) {
                version_ = entry.get<std::string>();
            }

            entry = config["executable"];

            if (entry.is_string()) {
                executablePath_ = build_sys_path(basePath_.c_str(), entry.get<std::string>().c_str(), NULL);
                log_trace("plugin [%s] executable [%s]", name_.c_str(), executablePath_.c_str());
            } else {
                if (type_ != helper::TYPE_INTERNAL) {
                    log_error("plugin [%s] has no executable", name_.c_str());
                    return PREP_FAILURE;
                }
            }

            return PREP_SUCCESS;
        }

        bool Plugin::is_enabled() const
        {
            return !executablePath_.empty();
        }

        Plugin::Result Plugin::on_load() const
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            return execute(LOAD);
        }

        Plugin::Result Plugin::on_unload() const
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            return execute(UNLOAD);
        }

        bool Plugin::is_valid() const
        {
            return is_enabled() && file_executable(executablePath_.c_str());
        }

        std::string Plugin::name() const
        {
            return name_;
        }

        std::string Plugin::type() const
        {
            return type_;
        }

        std::string Plugin::version() const
        {
            return version_;
        }

        bool Plugin::is_internal() const
        {
            return type_ == helper::TYPE_INTERNAL;
        }

        std::string Plugin::plugin_name(const Package &config) const
        {
            auto plugin = config.get_plugin_config(this);

            auto name = plugin["name"];

            if (name.is_string()) {
                return name.get<std::string>();
            }

            return config.name();
        }

        std::string Plugin::plugin_location(const Package &config) const
        {
            auto plugin = config.get_plugin_config(this);

            auto location = plugin["location"];

            if (location.is_string()) {
                return location.get<std::string>();
            }

            return "";
        }

        Plugin::Result Plugin::on_install(const Package &config, const std::string &path) const
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            if (strcasecmp(type().c_str(), "dependency")) {
                return PREP_FAILURE;
            }

            std::vector<std::string> info = {plugin_name(config), config.version(), path};

            return execute(INSTALL, info);
        }

        Plugin::Result Plugin::on_resolve(const Package &config) const
        {
            return on_resolve(plugin_location(config));
        }

        Plugin::Result Plugin::on_resolve(const std::string &location) const
        {
            char buf[PATH_MAX];

            if (!is_valid()) {
                return PREP_FAILURE;
            }

            if (strcasecmp(type().c_str(), "resolver")) {
                return PREP_FAILURE;
            }

            make_temp_dir(buf, PATH_MAX);

            std::vector<std::string> info = {buf, location};

            return execute(RESOLVE, info);
        }

        Plugin::Result Plugin::on_remove(const Package &config, const std::string &path) const
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            if (strcasecmp(type().c_str(), "dependency")) {
                return PREP_FAILURE;
            }

            std::vector<std::string> info = {plugin_name(config), config.version(), path};

            return execute(REMOVE, info);
        }

        Plugin::Result Plugin::on_build(const Package &config, const std::string &sourcePath,
                                        const std::string &buildPath, const std::string &installPath) const
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            if (strcasecmp(type().c_str(), "build")) {
                return PREP_FAILURE;
            }

            log_debug("Running [%s] for \x1b[1;35m%s\x1b[0m", name().c_str(), config.name().c_str());

            auto envVars = environment::build_cpp_variables();

            std::vector<std::string> info(
                {plugin_name(config), config.version(), sourcePath, buildPath, installPath, config.build_options()});

            info.insert(std::end(info), std::begin(envVars), std::end(envVars));

            return execute(BUILD, info);
        }

        Plugin::Result Plugin::execute(const Hooks &hook, const std::vector<std::string> &info) const
        {
            int master = 0;

            auto method = helper::HOOK_NAMES[hook];

            log_trace("executing [%s] on plugin [%s]", method, name_.c_str());

            // fork a psuedo terminal
            pid_t pid = forkpty(&master, NULL, NULL, NULL);

            if (pid < 0) {
                // respond to error
                log_errno(errno);
                return PREP_ERROR;
            }

            // if we're the child process...
            if (pid == 0) {
                // set the current directory to the plugin path
                if (chdir(basePath_.c_str())) {
                    log_error("unable to change directory [%s]", basePath_.c_str());
                    return PREP_FAILURE;
                }

                const char *argv[] = {name_.c_str(), nullptr};

                // execute the plugin in this process
                execvp(executablePath_.c_str(), (char *const *)argv);

                exit(PREP_FAILURE);  // exec never returns
            } else {
                // otherwise we are the parent process...
                int status = 0;
                std::vector<std::string> returnValues;
                struct termios tios;

                // set some terminal flags to remove local echo
                tcgetattr(master, &tios);
                tios.c_lflag &= ~(ECHO | ECHONL);
                tcsetattr(master, TCSAFLUSH, &tios);

                if (helper::write_header(master, method, info) == PREP_FAILURE) {
                    log_errno(errno);
                    if (kill(pid, SIGKILL) < 0) {
                        log_errno(errno);
                    }
                    return PREP_ERROR;
                }

                // start the io loop with child
                for (;;) {
                    fd_set read_fd;
                    fd_set write_fd;
                    fd_set except_fd;
                    char input = 0;
                    std::string line;
                    struct timeval poll = {0, 300000};

                    FD_ZERO(&read_fd);
                    FD_ZERO(&write_fd);
                    FD_ZERO(&except_fd);

                    FD_SET(master, &read_fd);
                    FD_SET(STDIN_FILENO, &read_fd);

                    // wait for something to happen
                    if (select(master + 1, &read_fd, &write_fd, &except_fd, &poll) < 0) {
                        if (errno != EINTR) {
                            log_errno(errno);
                        }
                        break;
                    }

                    vt100::update_progress();

                    // if we have something to read from child...
                    if (FD_ISSET(master, &read_fd)) {
                        // read a line
                        int n = helper::read_line(master, line);

                        if (n <= 0) {
                            if (n < 0) {
                                log_errno(errno);
                            }
                            break;
                        }

                        // if the line is a return value, add it, else print it
                        if (helper::is_return_command(line)) {
                            returnValues.push_back(line.substr(7));
                        } else if (verbose_ && helper::write_line(STDOUT_FILENO, line) < 0) {
                            log_errno(errno);
                            break;
                        }
                    }

                    // if we have something to read on stdin...
                    if (FD_ISSET(STDIN_FILENO, &read_fd)) {
                        int n = helper::read_line(STDIN_FILENO, line);

                        if (n <= 0) {
                            if (n < 0) {
                                log_errno(errno);
                            }
                            break;
                        }

                        // send it to the child
                        if (helper::write_line(master, line) < 0) {
                            log_errno(errno);
                            break;
                        }
                    }
                }

                // wait for the child to exit
                waitpid(pid, &status, 0);

                // check exit status of child
                if (WIFEXITED(status)) {
                    // convert the exit status to a return value
                    int rval = WEXITSTATUS(status);
                    // typically rval will be the return status of the command the plugin executes
                    rval = rval == 0 ? PREP_SUCCESS : rval == 2 ? PREP_ERROR : PREP_FAILURE;
                    return {rval, returnValues};
                } else if (WIFSIGNALED(status)) {
                    int sig = WTERMSIG(status);

                    // log signals
                    log_error("child process signal %d", sig);

                    // and core dump
                    if (WCOREDUMP(status)) {
                        log_error("child produced core dump");
                    }
                } else if (WIFSTOPPED(status)) {
                    int sig = WSTOPSIG(status);

                    log_error("child process stopped signal %d", sig);
                } else {
                    log_error("child process did not exit cleanly");
                }
            }

            return PREP_FAILURE;
        }
    }
}
