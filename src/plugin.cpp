
#include <fstream>
#include <csignal>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <iterator>
#include <util.h>
#include <vector>

#include "common.h"
#include "package.h"
#include "environment.h"
#include "log.h"
#include "plugin.h"
#include "util.h"

namespace micrantha
{
    namespace prep
    {
        namespace internal
        {
            constexpr const char *const TYPE_NAMES[] = { "internal", "configuration", "dependency", "resolver", "build" };

            std::string to_string(Plugin::Hooks hook) {
                switch(hook) {
                    case Plugin::Hooks::LOAD:
                        return "load";
                    case Plugin::Hooks::UNLOAD:
                        return "unload";
                    case Plugin::Hooks::ADD:
                        return "add";
                    case Plugin::Hooks::REMOVE:
                        return "remove";
                    case Plugin::Hooks::RESOLVE:
                        return "resolve";
                    case Plugin::Hooks::BUILD:
                        return "build";
                    case Plugin::Hooks::TEST:
                        return "test";
                    case Plugin::Hooks::INSTALL:
                        return "install";
                }
            }

            std::string to_string(Plugin::Types type) {
                return TYPE_NAMES[static_cast<int>(type)];
            }


            Plugin::Types to_type(const std::string &type) {
                for(int i = 0; i < sizeof(TYPE_NAMES) / sizeof(TYPE_NAMES[0]); i++) {
                    if (strcasecmp(TYPE_NAMES[i], type.c_str()) == 0) {
                        return static_cast<Plugin::Types>(i);
                    }
                }
                return Plugin::Types::INTERNAL;
            }

            bool is_valid_type(Plugin::Types type) {
                return type != Plugin::Types::INTERNAL;
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

            class Interpreter
            {
              public:
                std::vector<std::string> returns;

                bool interpret(const std::string &line)
                {
                    if (is_return_command(line)) {
                        returns.push_back(line.substr(7));
                        return true;
                    }

                    if (is_echo_command(line)) {
                        log::info(line.substr(5));
                        return true;
                    }

                    return false;
                }

              private:
                // test input for a return command
                static bool is_return_command(const std::string &line)
                {
                    return line.length() > 7 && !strcasecmp(line.substr(0, 7).c_str(), "RETURN ");
                }

                // test input for a echo command
                static bool is_echo_command(const std::string &line)
                {
                    return line.length() > 5 && !strcasecmp(line.substr(0, 5).c_str(), "ECHO ");
                }
            };

            std::string get_plugin_string(const std::string &plugin, const std::string &key, const Package &config)
            {
                auto json = config.get_value(plugin);

                auto value = json[key];

                if (value.is_string()) {
                    return value.get<std::string>();
                }

                value = config.get_value(key);

                if (value.is_string()) {
                    return value;
                }

                return "";
            }
        }

        std::ostream &operator<<(std::ostream &out, const Plugin::Result &result) {
            out << result.code;
            if (!result.values.empty()) {
                out << ":";
                std::copy(result.values.begin(), result.values.end(), std::ostream_iterator<std::string>(out, ","));
            }
            return out;
        }

        Plugin::Plugin(const std::string &name) : name_(name), type_(Types::INTERNAL)
        {
        }

        Plugin::~Plugin()
        {
            on_unload();
        }

        Plugin &Plugin::set_verbose(bool value)
        {
            verbose_ = value;
            return *this;
        }

        int Plugin::load(const std::string &path)
        {
            basePath_ = path;

            std::string manifest = build_sys_path(path, MANIFEST_FILE);

            std::ifstream file(manifest);

            if (!file.is_open()) {
                return PREP_ERROR;
            }

            std::ostringstream buf;

            file >> buf.rdbuf();

            auto config = Package::json_type::parse(buf.str().c_str());

            if (config.empty()) {
                log::error("invalid configuration for plugin [", name_, "]");
                return PREP_FAILURE;
            }

            auto entry = config["type"];

            if (entry.is_string()) {
                type_ = internal::to_type(entry.get<std::string>());
            }

            entry = config["version"];

            if (entry.is_string()) {
                version_ = entry.get<std::string>();
            }

            entry = config["executable"];

            if (entry.is_string()) {
                executablePath_ = build_sys_path(basePath_, entry.get<std::string>());
                log::trace("plugin [", name_, "] executable [", executablePath_, "]");
            } else {
                if (type_ != Types::INTERNAL) {
                    log::error("plugin [", name_, "] has no executable");
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

            return execute(Hooks::LOAD);
        }

        Plugin::Result Plugin::on_unload() const
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            return execute(Hooks::UNLOAD);
        }

        bool Plugin::is_valid() const
        {
            return is_enabled() && file_executable(executablePath_);
        }

        std::string Plugin::name() const
        {
            return name_;
        }

        Plugin::Types Plugin::type() const
        {
            return type_;
        }

        std::string Plugin::version() const
        {
            return version_;
        }

        Plugin::Result Plugin::on_add(const Package &config, const std::string &path) const
        {
            if (!is_valid()) {
                return PREP_ERROR;
            }

            if (type_ != Types::DEPENDENCY) {
                return PREP_ERROR;
            }

            std::vector<std::string> info = { internal::get_plugin_string(name(), "name", config), config.version(), path };

            return execute(Hooks::ADD, info);
        }

        Plugin::Result Plugin::on_resolve(const Package &config, const std::string &sourcePath) const
        {
            return on_resolve(internal::get_plugin_string(name(), "location", config), sourcePath);
        }

        Plugin::Result Plugin::on_resolve(const std::string &location, const std::string &sourcePath) const
        {
            if (!is_valid()) {
                return PREP_ERROR;
            }

            if (type_ != Types::RESOLVER) {
                return PREP_ERROR;
            }

            std::vector<std::string> info = { sourcePath, location };

            return execute(Hooks::RESOLVE, info);
        }

        Plugin::Result Plugin::on_remove(const Package &config, const std::string &path) const
        {
            if (!is_valid()) {
                return PREP_ERROR;
            }

            if (type_ != Types::DEPENDENCY) {
                return PREP_ERROR;
            }

            std::vector<std::string> info = { internal::get_plugin_string(name(), "name", config), config.version(), path };

            return execute(Hooks::REMOVE, info);
        }

        Plugin::Result Plugin::on_build(const Package &config, const std::string &sourcePath, const std::string &buildPath, const std::string &installPath) const
        {
            if (!is_valid()) {
                return PREP_ERROR;
            }

            if (type_ != Types::BUILD && type_ != Types::CONFIGURATION) {
                return PREP_ERROR;
            }

            log::debug("Running [", name(), "] for ", config.name());

            std::vector<std::string> info(
                { internal::get_plugin_string(name(), "name", config), config.version(), sourcePath, buildPath, installPath, config.build_options() });

            auto env = environment::build_env();

            info.insert(info.end(), env.begin(), env.end());

            return execute(Hooks::BUILD, info);
        }

        Plugin::Result Plugin::on_test(const Package &config, const std::string &sourcePath, const std::string &buildPath) const
        {
            if (!is_valid()) {
                return PREP_ERROR;
            }

            if (type_ != Types::BUILD) {
                return PREP_ERROR;
            }

            log::debug("Running [", name(), "] for ", config.name());

            std::vector<std::string> info(
                    { internal::get_plugin_string(name(), "name", config), config.version(), sourcePath, buildPath });

            auto env = environment::build_env();

            info.insert(info.end(), env.begin(), env.end());

            return execute(Hooks::TEST, info);
        }

        Plugin::Result Plugin::on_install(const Package &config, const std::string &sourcePath, const std::string &buildPath) const
        {
            if (!is_valid()) {
                return PREP_ERROR;
            }

            if (type_ != Types::BUILD) {
                return PREP_ERROR;
            }

            log::debug("Running [", name(), "] for ", config.name());

            std::vector<std::string> info(
                    { internal::get_plugin_string(name(), "name", config), config.version(), sourcePath, buildPath });

            auto env = environment::build_env();

            info.insert(info.end(), env.begin(), env.end());

            return execute(Hooks::INSTALL, info);
        }

        Plugin::Result Plugin::execute(const Hooks &hook, const std::vector<std::string> &info) const
        {
            int master = 0;

            auto method = internal::to_string(hook);

            log::trace("executing [", method, "] on plugin [", name_, "]");

            // fork a psuedo terminal
            pid_t pid = forkpty(&master, nullptr, nullptr, nullptr);

            if (pid < 0) {
                // respond to error
                log::perror(errno);
                return PREP_ERROR;
            }

            // if we're the child process...
            if (pid == 0) {
                // set the current directory to the plugin path
                if (chdir(basePath_.c_str())) {
                    log::error("unable to change directory [", basePath_, "]");
                    return PREP_FAILURE;
                }

                const char *argv[] = { name_.c_str(), nullptr };

                // execute the plugin in this process
                execvp(executablePath_.c_str(), (char *const *)argv);

                exit(PREP_FAILURE); // exec never returns
            } else {
                // otherwise we are the parent process...
                int status = 0;
                internal::Interpreter interpreter;
                struct termios tios = {};

                // set some terminal flags to remove local echo
                tcgetattr(master, &tios);
                tios.c_lflag &= ~(ECHO | ECHONL | ECHOCTL);
                tcsetattr(master, TCSAFLUSH, &tios);

                if (internal::write_header(master, method, info) == PREP_FAILURE) {
                    log::perror(errno);
                    if (kill(pid, SIGKILL) < 0) {
                        log::perror(errno);
                    }
                    return PREP_ERROR;
                }

                // start the io loop with child
                for (;;) {
                    fd_set read_fd = {};
                    fd_set write_fd = {};
                    fd_set except_fd;
                    std::string line;

                    FD_ZERO(&read_fd);
                    FD_ZERO(&write_fd);
                    FD_ZERO(&except_fd);

                    FD_SET(master, &read_fd);
                    FD_SET(STDIN_FILENO, &read_fd);
                    FD_SET(STDERR_FILENO, &read_fd);

                    // wait for something to happen
                    if (select(master + 1, &read_fd, &write_fd, &except_fd, nullptr) < 0) {
                        if (errno != EINTR) {
                            log::perror(errno);
                        }
                        break;
                    }

                    // if we have something to read from child...
                    if (FD_ISSET(master, &read_fd)) {
                        // read a line
                        ssize_t n = internal::read_line(master, line);

                        if (n <= 0) {
                            if (n < 0) {
                                log::perror(errno);
                            }
                            break;
                        }

                        // if the line is a return value, add it, else print it
                        if (!interpreter.interpret(line)) {
                            if (verbose_ && internal::write_line(STDOUT_FILENO, line) < 0) {
                                log::perror(errno);
                                break;
                            }
                        }
                    }

                    // if we have something to read on stdin...
                    if (FD_ISSET(STDIN_FILENO, &read_fd)) {
                        ssize_t n = internal::read_line(STDIN_FILENO, line);

                        if (n <= 0) {
                            if (n < 0) {
                                log::perror(errno);
                            }
                            break;
                        }

                        // send it to the child
                        if (internal::write_line(master, line) < 0) {
                            log::perror(errno);
                            break;
                        }
                    }

                    // if we have something to read on stdin...
                    if (FD_ISSET(STDERR_FILENO, &read_fd)) {
                        ssize_t n = internal::read_line(STDERR_FILENO, line);

                        if (n <= 0) {
                            if (n < 0) {
                                log::perror(errno);
                            }
                            break;
                        }

                        // send it to the child
                        if (internal::write_line(master, line) < 0) {
                            log::perror(errno);
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
                    return { rval, interpreter.returns };
                } else if (WIFSIGNALED(status)) {
                    int sig = WTERMSIG(status);

                    // log signals
                    log::error("child process signal ", sig);

                    // and core dump
                    if (WCOREDUMP(status)) {
                        log::error("child produced core dump");
                    }
                } else if (WIFSTOPPED(status)) {
                    int sig = WSTOPSIG(status);

                    log::error("child process stopped signal ", sig);
                } else {
                    log::error("child process did not exit cleanly");
                }
            }

            return PREP_FAILURE;
        }
    }
}
