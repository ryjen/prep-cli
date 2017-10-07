//
// Created by Ryan Jennings on 2017-10-07.
//

#ifndef MICRANTHA_PREP_VT100_H
#define MICRANTHA_PREP_VT100_H

#include <thread>
#include <string>
#include <vector>

namespace micrantha {
    namespace prep {

        namespace vt100 {
            namespace cursor {
                constexpr static const char *const BACK = "\033[1D";
                constexpr static const char *const SAVE = "\0337";
                constexpr static const char *const RESTORE = "\0338";
                constexpr static const char *const ERASE_LINE = "\033[2K";

                void set(int rows, int cols);
            }

            void init();

            class Progress {
            public:
                Progress() noexcept;

                ~Progress();

                Progress(const Progress &) = delete;

                Progress(Progress &&) = delete;

            private:
                void update();

                void reset();

                void run();

                bool alive_;
                std::thread bg_;
            };
        }
        namespace color {
            typedef unsigned Value;

            namespace fg {
                typedef enum {
                    BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE
                } Type;
            }
            namespace bg {
                typedef enum {
                    BLACK = 40, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE
                } Type;
            }

            namespace attr {
                typedef enum {
                    NORMAL, BOLD
                } Type;
            }

            using Foreground = fg::Type;
            using Background = bg::Type;
            using Attribute = attr::Type;

            constexpr static const char *const CLEAR = "\033[0m";

            std::string colorize(const std::vector <Value> &colors, const std::string &value, bool reset = true);

            std::string B(const std::string &value);

            std::string r(const std::string &value);

            std::string g(const std::string &value);

            std::string y(const std::string &value);

            std::string b(const std::string &value);

            std::string m(const std::string &value);

            std::string c(const std::string &value);

            std::string w(const std::string &value);
        }

    }
}

#endif //PREP_VT100_H
