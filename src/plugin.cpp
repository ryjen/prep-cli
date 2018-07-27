
#include <fstream>
#include <csignal>
#include <sstream>
#include <thread>
#include <pty.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "common.h"
#include "package.h"
#include "environment.h"
#include "log.h"
#include "plugin.h"

namespace micrantha {
  namespace prep {
    namespace internal {
      constexpr const char *const END_HEADER = "END";

      constexpr const char *const TYPE_NAMES[] = {"internal", "configuration", "dependency", "resolver", "build"};

      std::string to_string(Plugin::Hooks hook) {
        switch (hook) {
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
        for (int i = 0; i < sizeof(TYPE_NAMES) / sizeof(TYPE_NAMES[0]); i++) {
          if (strcasecmp(TYPE_NAMES[i], type.c_str()) == 0) {
            return static_cast<Plugin::Types>(i);
          }
        }
        return Plugin::Types::INTERNAL;
      }

      bool is_valid_type(Plugin::Types type) {
        return type != Plugin::Types::INTERNAL;
      }

      // writes a header to the parent process, resulting as input to child process
      int write_header(int fd, const std::string &method, const std::vector<std::string> &info) {
        // write the hook method to the plugin (child)
        if (io::write_line(fd, method) < 0) {
          return PREP_FAILURE;
        }

        // for each additional argument...
        for (auto &i : info) {
          // write to the child
          if (io::write_line(fd, i) < 0) {
            return PREP_FAILURE;
          }
        }

        // write the header terminator
        if (io::write_line(fd, END_HEADER) < 0) {
          return PREP_FAILURE;
        }

        return PREP_SUCCESS;
      }


      /**
       * A very naive interpreter.  TODO: enlighten
       */
      class Interpreter {
        public:
          std::vector<std::string> returns;

          Interpreter(bool verbose = false) : verbose_(verbose) {}

          /**
           * @return true if line should be output
           */
          int interpret(const std::string &line) {
            if (is_return_command(line)) {
              returns.push_back(line.substr(7));
              return PREP_SUCCESS;
            }

            if (is_echo_command(line)) {
              if (io::write_line(STDOUT_FILENO, line.substr(5)) < 0) {
                return PREP_ERROR;
              }
              return PREP_SUCCESS;
            }

            if (verbose_) {
              if (io::write_line(STDOUT_FILENO, line) < 0) {
                return PREP_ERROR;
              }
              return PREP_SUCCESS;
            }

            return PREP_SUCCESS;
          }

        private:
          bool verbose_;

          // test input for a return command
          static bool is_return_command(const std::string &line) {
            return line.length() > 7 && !strcasecmp(line.substr(0, 7).c_str(), "RETURN ");
          }

          // test input for a echo command
          static bool is_echo_command(const std::string &line) {
            return line.length() > 5 && !strcasecmp(line.substr(0, 5).c_str(), "ECHO ");
          }
      };

      std::string get_plugin_string(const std::string &plugin, const std::string &key, const Package &config) {
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

    Plugin::Plugin(const std::string &name) : name_(name), type_(Types::INTERNAL), enabled_(true) {
    }

    Plugin::~Plugin() {
      on_unload();
    }

    Plugin &Plugin::set_verbose(bool value) {
      verbose_ = value;
      return *this;
    }

    Plugin &Plugin::set_enabled(bool value) {
      config_["enabled"] = enabled_ = value;
      return *this;
    }

    int Plugin::read_config() {

      std::string manifest = filesystem::build_path(basePath_, MANIFEST_FILE);

      std::ifstream file(manifest);

      if (!file.is_open()) {
        return PREP_ERROR;
      }

      std::ostringstream buf;

      file >> buf.rdbuf();

      config_ = Package::json_type::parse(buf.str().c_str());

      if (config_.empty()) {
        log::error("invalid configuration for plugin [", name_, "]");
        return PREP_FAILURE;
      }

      auto entry = config_["type"];

      if (entry.is_string()) {
        type_ = internal::to_type(entry.get<std::string>());
      }

      entry = config_["version"];

      if (entry.is_string()) {
        version_ = entry.get<std::string>();
      }

      entry = config_["enabled"];

      if (entry.is_boolean()) {
        enabled_ = entry.get<bool>();
      }

      entry = config_["executable"];

      if (entry.is_string()) {
        executablePath_ = filesystem::build_path(basePath_, entry.get<std::string>());
      }

      return PREP_SUCCESS;
    }

    int Plugin::save() {

      std::string manifest = filesystem::build_path(basePath_, MANIFEST_FILE);

      std::ofstream file(manifest);

      if (!file.is_open()) {
        return PREP_ERROR;
      }

      file << config_;

      file.close();

      return PREP_SUCCESS;
    }

    int Plugin::load(const std::string &path) {
      basePath_ = path;

      read_config();

      if (executablePath_.empty() && type_ != Types::INTERNAL) {
        log::error("plugin [", name_, "] has no executable");
        return PREP_FAILURE;

      }

      return PREP_SUCCESS;
    }

    bool Plugin::is_enabled() const {
      return enabled_;
    }

    bool Plugin::is_valid() const {
      return filesystem::is_file_executable(executablePath_);
    }

    std::string Plugin::name() const {
      return name_;
    }

    Plugin::Types Plugin::type() const {
      return type_;
    }

    std::string Plugin::version() const {
      return version_;
    }

    Plugin::Result Plugin::on_load() const {
      if (!is_valid() || !is_enabled()) {
        return PREP_FAILURE;
      }

      log::debug("loading ", color::m(name()), " [", color::y(version()), "]");

      return execute(Hooks::LOAD);
    }

    Plugin::Result Plugin::on_unload() const {
      if (!is_valid() || !is_enabled()) {
        return PREP_FAILURE;
      }

      return execute(Hooks::UNLOAD);
    }

    Plugin::Result Plugin::on_add(const Package &config, const std::string &path) const {
      if (!is_valid() || !is_enabled()) {
        return PREP_ERROR;
      }

      if (type_ != Types::DEPENDENCY) {
        return PREP_ERROR;
      }

      std::vector<std::string> info = {internal::get_plugin_string(name(), "name", config), config.version(),
        path};

      return execute(Hooks::ADD, info);
    }

    Plugin::Result Plugin::on_resolve(const Package &config, const std::string &sourcePath) const {
      return on_resolve(internal::get_plugin_string(name(), "location", config), sourcePath);
    }

    Plugin::Result Plugin::on_resolve(const std::string &location, const std::string &sourcePath) const {
      if (!is_valid() || !is_enabled()) {
        return PREP_ERROR;
      }

      if (type_ != Types::RESOLVER) {
        return PREP_ERROR;
      }

      std::vector<std::string> info = {sourcePath, location};

      return execute(Hooks::RESOLVE, info);
    }

    Plugin::Result Plugin::on_remove(const Package &config, const std::string &path) const {
      if (!is_valid() || !is_enabled()) {
        return PREP_ERROR;
      }

      if (type_ != Types::DEPENDENCY) {
        return PREP_ERROR;
      }

      std::vector<std::string> info = {internal::get_plugin_string(name(), "name", config), config.version(),
        path};

      return execute(Hooks::REMOVE, info);
    }

    Plugin::Result Plugin::on_build(const Package &config, const std::string &sourcePath, const std::string &buildPath,
        const std::string &installPath) const {
      if (!is_valid() || !is_enabled()) {
        return PREP_ERROR;
      }

      if (type_ != Types::BUILD && type_ != Types::CONFIGURATION) {
        return PREP_ERROR;
      }

      log::info("building ", color::m(config.name()), " with ", color::c(name()));

      std::vector<std::string> info(
          {internal::get_plugin_string(name(), "name", config), config.version(), sourcePath, buildPath,
          installPath, config.build_options()});

      auto env = environment::build_env();

      info.insert(info.end(), env.begin(), env.end());

      return execute(Hooks::BUILD, info);
    }

    Plugin::Result Plugin::on_test(const Package &config, const std::string &sourcePath, const std::string &buildPath) const {
      if (!is_valid() || !is_enabled()) {
        return PREP_ERROR;
      }

      if (type_ != Types::BUILD) {
        return PREP_ERROR;
      }

      log::info("testing ", color::m(config.name()), " with ", color::c(name()));

      std::vector<std::string> info(
          {internal::get_plugin_string(name(), "name", config), config.version(), sourcePath, buildPath});

      auto env = environment::build_env();

      info.insert(info.end(), env.begin(), env.end());

      return execute(Hooks::TEST, info);
    }

    Plugin::Result Plugin::on_install(const Package &config, const std::string &sourcePath, const std::string &buildPath) const {
      if (!is_valid() || !is_enabled()) {
        return PREP_ERROR;
      }

      if (type_ != Types::BUILD) {
        return PREP_ERROR;
      }

      log::info("installing ", color::m(config.name()), " with ", color::c(name()));

      std::vector<std::string> info(
          {internal::get_plugin_string(name(), "name", config), config.version(), sourcePath, buildPath});

      auto env = environment::build_env();

      info.insert(info.end(), env.begin(), env.end());

      return execute(Hooks::INSTALL, info);
    }

    Plugin::Result Plugin::execute(const Hooks &hook, const std::vector<std::string> &info) const {
      int master = 0;

      // fork a psuedo terminal
      pid_t pid = forkpty(&master, nullptr, nullptr, nullptr);

      if (pid < 0) {
        // respond to error
        log::perror("forkpty");
        return PREP_ERROR;
      }

      // if we're the child process...
      if (pid == 0) {

        // set the current directory to the plugin path
        if (chdir(basePath_.c_str())) {
          log::error("unable to change directory [", basePath_, "]");
          return PREP_FAILURE;
        }

        const char *argv[] = {name_.c_str(), nullptr};

        // execute the plugin in this process
        execvp(executablePath_.c_str(), (char *const *) argv);

        exit(PREP_FAILURE); // exec never returns
      } else {
        // otherwise we are the parent process...
        int status = 0;
        internal::Interpreter interpreter(verbose_);
        struct termios tios = {};

        // set some terminal flags to remove local echo
        tcgetattr(master, &tios);
        tios.c_lflag &= ~(ECHO | ECHONL | ECHOCTL);
        tcsetattr(master, TCSAFLUSH, &tios);

        auto method = internal::to_string(hook);

        log::trace("executing [", method, "] on plugin [", name_, "]");

        if (internal::write_header(master, method, info) == PREP_FAILURE) {
          log::perror("write_header");
          if (kill(pid, SIGKILL) < 0) {
            log::perror("kill");
          }
          return PREP_ERROR;
        }

        // start the io loop with child
        for (;;) {
          fd_set read_fd = {};
          std::string line;

          FD_ZERO(&read_fd);

          FD_SET(master, &read_fd);
          FD_SET(STDIN_FILENO, &read_fd);

          // wait for something to happen
          if (select(master + 1, &read_fd, nullptr, nullptr, nullptr) < 0) {
            if (errno == EINTR) {
              continue;
            }
            log::perror("select");
            break;
          }

          // if we have something to read from child...
          if (FD_ISSET(master, &read_fd)) {
            // read a line
            ssize_t n = io::read_line(master, line);

            if (n <= 0) {
              if (n < 0 && errno != EIO) {
                log::perror("read_line");
              }
              break;
            }

            if (interpreter.interpret(line) != PREP_SUCCESS) {
              log::perror("interpret");
              break;
            }
          }

          // if we have something to read on stdin...
          if (FD_ISSET(STDIN_FILENO, &read_fd)) {
            ssize_t n = io::read_line(STDIN_FILENO, line);

            if (n <= 0) {
              if (n < 0) {
                log::perror("read_line");
              }
              break;
            }

            // send it to the child
            if (io::write_line(master, line) < 0) {
              log::perror("write_line");
              break;
            }
          }
        }

        // wait for the child to exit
        pid = waitpid(pid, &status, WAIT_MYPGRP|WUNTRACED);

        if (pid == -1) {
          log::perror("error waiting for plugin");
          return PREP_FAILURE;
        }

        // check exit status of child
        if (WIFEXITED(status)) {
          // convert the exit status to a return value
          int rval = WEXITSTATUS(status);
          // typically rval will be the return status of the command the plugin executes, 255 = -1
          rval = rval == 255 ? PREP_ERROR : rval;
          return {rval, interpreter.returns};
        } else if (WIFSIGNALED(status)) {
          int sig = WTERMSIG(status);

          // log signals
          log::error(name(), " terminated signal ", sig);

          // and core dump
          if (WCOREDUMP(status)) {
            log::error(name(), " produced core dump");
          }
        } else if (WIFSTOPPED(status)) {
          int sig = WSTOPSIG(status);

          log::error(name(), " stopped signal ", sig);
        } else {
          log::error(name(), " did not exit cleanly");
        }
      }

      return PREP_FAILURE;
    }
  }
}
