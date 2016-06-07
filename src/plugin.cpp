#include <unistd.h>
#include <util.h>
#include <thread>
#include <signal.h>
#include <sstream>
#include "plugin.h"
#include "common.h"
#include "log.h"
#include "environment.h"
#include "util.h"

namespace arg3
{
    namespace prep
    {
        plugin::plugin(const std::string &name) : name_(name) {}

        plugin::~plugin() {
            on_unload();
        }

        int plugin::load(const std::string &path) {
            basePath_ = path;

            std::string manifest = build_sys_path(path.c_str(), "manifest.json", NULL);

            // skip folders with no manifest
            if (!file_exists(manifest.c_str())) {
                throw path;
            }

            std::ifstream file;
            std::ostringstream buf;

            file.open(manifest);

            file >> buf.rdbuf();

            json_object *config = json_tokener_parse(buf.str().c_str());

            if (config == NULL) {
                log_error("invalid configuration for plugin [%s]", name_.c_str());
                return PREP_FAILURE;
            }

            json_object *obj = NULL;

            if (json_object_object_get_ex(config, "executable", &obj)) {
                executablePath_ = build_sys_path(basePath_.c_str(), json_object_get_string(obj), NULL);
                log_trace("plugin [%s] executable [%s]", name_.c_str(), executablePath_.c_str());
            } else {
                log_error("plugin [%s] has no executable", name_.c_str());
                return PREP_FAILURE;
            }

            json_object_put(obj);

            if (json_object_object_get_ex(config, "version", &obj)) {
                version_ = json_object_get_string(obj);
            }

            json_object_put(obj);

            json_object_put(config);

            log_info("loaded plugin [%s] version [%s]", name_.c_str(), version_.c_str());

            return PREP_SUCCESS;
        }

        bool plugin::is_enabled() const {
            return !executablePath_.empty();
        }
        int plugin::on_load()
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            return execute("load");
        }

        int plugin::on_unload()
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            return execute("unload");
        }

        bool plugin::is_valid() const {
            return is_enabled() && can_exec_file(executablePath_);
        }

        std::string plugin::name() const {
            return name_;
        }

        int plugin::on_install(const package &config)
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            std::vector<std::string> info = {
                config.get_plugin_name(this), config.version()
            };

            return execute("install", info);
        }

        int plugin::on_remove(const package &config)
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            std::vector<std::string> info = {
                config.get_plugin_name(this), config.version()
            };

            return execute("remove", info);
        }

        int plugin::on_build(const package &config, const std::string &sourcePath, const std::string &buildPath, const std::string &installPath)
        {
            if (!is_valid()) {
                return PREP_FAILURE;
            }

            auto envVars = environment::build_cpp_variables();

            std::vector<std::string> info({
                config.get_plugin_name(this), config.version(), sourcePath, buildPath, installPath, config.build_options()
            });

            info.insert(std::end(info), std::begin(envVars), std::end(envVars));

            return execute("build", info);
        }

        int plugin::execute(const std::string &method, const std::vector<std::string> &info)
        {
            int master = 0;

            log_trace("executing [%s] on plugin [%s]", method.c_str(), name_.c_str());

            pid_t pid = forkpty(&master, NULL, NULL, NULL);

            if (pid < 0) {
                log_errno(errno);
                return PREP_FAILURE;
            }

            if (pid == 0) {
                if (chdir(basePath_.c_str())) {
                    log_error("unable to change directory [%s]", basePath_.c_str());
                    return PREP_FAILURE;
                }

                const char *argv[] = {name_.c_str(), nullptr};

                // we are the child
                execvp(executablePath_.c_str(), (char*const*) argv);

                exit(PREP_FAILURE);  // exec never returns
            } else {
                int status = 0;

                struct termios tios;
                tcgetattr(master, &tios);
                tios.c_lflag &= ~(ECHO | ECHONL);
                tcsetattr(master, TCSAFLUSH, &tios);

                sleep(1);

                if (write(master, method.c_str(), method.length()) < 0 ||
                    write(master, "\n", 1) < 0) {
                    log_errno(errno);
                    if (kill(pid, SIGKILL) < 0) {
                        log_errno(errno);
                    }
                    return PREP_FAILURE;
                }

                for(auto &i : info) {
                    if (write(master, i.c_str(), i.length()) < 0 ||
                        write(master, "\n", 1) < 0) {
                        log_errno(errno);
                        if (kill(pid, SIGKILL) < 0) {
                            log_errno(errno);
                        }
                        return PREP_FAILURE;
                    }
                }

                if (write(master, "END", 3) < 0 ||
                    write(master, "\n", 1) < 0) {

                    log_errno(errno);
                    if (kill(pid, SIGKILL) < 0) {
                        log_errno(errno);
                    }
                    return PREP_FAILURE;
                }

                for (;;) {
                    fd_set read_fd;
                    fd_set write_fd;
                    fd_set except_fd;
                    char input = 0;
                    char output = 0;

                    FD_ZERO(&read_fd);
                    FD_ZERO(&write_fd);
                    FD_ZERO(&except_fd);

                    FD_SET(master, &read_fd);
                    FD_SET(STDIN_FILENO, &read_fd);

                    if (select(master+1, &read_fd, &write_fd, &except_fd, NULL) < 0) {
                        log_errno(errno);
                        break;
                    }

                    if (FD_ISSET(master, &read_fd))
                    {
                        int n = read(master, &output, 1);

                        if (n <= 0) {
                            if (n < 0) {
                                log_errno(errno);
                            }
                            break;
                        }

                        if (write(STDOUT_FILENO, &output, 1) < 0) {
                            log_errno(errno);
                            break;
                        }
                    }

                    if (FD_ISSET(STDIN_FILENO, &read_fd))
                    {
                        int n = read(STDIN_FILENO, &input, 1);

                        if (n <= 0) {
                            if (n < 0) {
                                log_errno(errno);
                            }
                            break;
                        }
                        if (write(master, &input, 1) < 0) {
                            log_errno(errno);
                            break;
                        }
                    }
                }

                waitpid(pid, &status, 0);

                if (WIFEXITED(status)) {
                    int rval = WEXITSTATUS(status);
                    return rval == 0 ? PREP_SUCCESS : PREP_FAILURE;
                } else if (WIFSIGNALED(status)) {
                    int sig = WTERMSIG(status);

                    log_error("child process signal %d", sig);

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
