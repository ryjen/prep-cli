//
// Created by Ryan Jennings on 2017-10-07.
//

#include "vt100.h"

#include <sstream>
#include <iostream>
#include <sys/termios.h>
#include <unistd.h>

#include "log.h"

namespace micrantha {
    namespace prep {


        namespace internal {
            // indicates good terminal support
            bool valid_term;
            int rows = 0, cols = 0;

            // tests a TERM value for validity
            bool init(const char *term) {
                static const std::string term_names[] = {"xterm", "ansi", "linux", "vt100"};
                if (term == nullptr) {
                    return false;
                }
                log::trace("found ", term, " terminal");
                std::string test(term);
                for (const auto &term : term_names) {
                    if (term.find("xterm") != std::string::npos) {
                        return true;
                    }
                }
                return false;
            }
        }
        namespace color {
            std::string colorize(const std::vector<Value> &colors, const std::string &value, bool reset) {

                // don't colorize if unknown term
                if (!internal::valid_term) {
                    return value;
                }

                std::ostringstream buf;

                buf << "\033[";

                for (auto it = colors.begin(); it != colors.end(); ++it) {
                    buf << *it;
                    if (it != --colors.end()) {
                        buf << ";";
                    }
                }
                buf << "m" << value;

                if (reset) {
                    buf << color::CLEAR;
                }
                return buf.str();
            }

            std::string B(const std::string &value) {
                return colorize({fg::BLACK}, value, true);
            }

            std::string r(const std::string &value) {
                return colorize({fg::RED}, value, true);
            }

            std::string g(const std::string &value) {
                return colorize({fg::GREEN}, value, true);
            }

            std::string y(const std::string &value) {
                return colorize({fg::YELLOW}, value, true);
            }

            std::string b(const std::string &value) {
                return colorize({fg::BLUE}, value, true);
            }

            std::string m(const std::string &value) {
                return colorize({fg::MAGENTA}, value, true);
            }

            std::string c(const std::string &value) {
                return colorize({fg::CYAN}, value, true);
            }

            std::string w(const std::string &value) {
                return colorize({fg::WHITE}, value, true);
            }
        }

        // other terminal codes
        namespace vt100 {

            namespace output {
                std::ostream &print(std::ostream &os) {
                    return os;
                }

                std::mutex &get_mutex() {
                    static std::mutex m;
                    return m;
                }
            }
            namespace cursor {

                void set(int rows, int cols) {
                    if (internal::valid_term && rows > 0 && cols > 0) {
                        std::cout << "\033[" << rows << ";" << cols << "H" << std::flush;
                    }
                }

                void get(int *row, int *col) {
                    if (!internal::valid_term) {
                        return;
                    }

                    struct termios ttystate, ttysave;

                    tcgetattr(STDIN_FILENO, &ttystate);
                    ttysave = ttystate;
                    //turn off canonical mode and echo
                    ttystate.c_lflag &= ~(ICANON | ECHO);
                    //minimum of number input read.
                    ttystate.c_cc[VMIN] = 1;
                    ttystate.c_cc[VTIME] = 2;

                    //set the terminal attributes.
                    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

                    std::cout << cursor::REPORT << std::flush;

                    fscanf(stdin, "\033[%d;%dR", row, col);

                    tcsetattr(STDIN_FILENO, TCSANOW, &ttysave);

                };

                Savepoint::Savepoint() {
                    std::cout << SAVE << std::flush;
                }

                Savepoint::~Savepoint() {
                    std::cout << RESTORE << std::flush;
                }
            }

            void init() {
                auto term = std::getenv("TERM");
                internal::valid_term = term != nullptr && internal::init(term);

                if (internal::valid_term) {

                    cursor::Savepoint savepoint;

                    // find the screen height by setting the cursor to a extreme value
                    cursor::set(99999, 99999);

                    cursor::get(&internal::rows, &internal::cols);
                }
            }

            Progress::Progress() noexcept : alive_(true), row_(0),
                                            bg_(std::bind(&Progress::run, this)) {

            }

            Progress::~Progress() {
                alive_ = false;
                if (bg_.joinable()) {
                    bg_.join();
                }
                reset();
            }

            void Progress::reset() {
                if (!internal::valid_term) {
                    return;
                }

                cursor::Savepoint savepoint;

                // restore the cursor to the progress indicator
                cursor::set(row_ - 1, 1);

                // and erase the line
                std::cout << " " << std::flush;

            }

            void Progress::run() {
                if (!internal::valid_term) {
                    return;
                }

                int col = 0;

                cursor::get(&row_, &col);

                using namespace std::chrono_literals;

                while (internal::valid_term && alive_) {
                    update();
                    std::this_thread::sleep_for(100ms);
                }
            }

            void Progress::update() {
                static unsigned frame = 0;
                constexpr static const char *FRAMES[] = {"◜", "◠", "◝", "◞", "◡", "◟"};
                constexpr static size_t FRAME_SIZE = (sizeof(FRAMES) / sizeof(FRAMES[0]));

                ++frame;
                frame %= FRAME_SIZE;

                if (internal::valid_term) {

                    cursor::Savepoint savepoint;

                    // set the cursor to the saved position
                    cursor::set(row_ - 1, 1);

                    // send the frame and move the cursor back one character to print again
                    std::cout << color::colorize({color::attr::BOLD, color::fg::GREEN}, FRAMES[frame], true)
                              << std::flush;
                }
            }
        }
    }
}