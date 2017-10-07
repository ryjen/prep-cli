//
// Created by Ryan Jennings on 2017-10-07.
//

#include "vt100.h"

#include <sstream>
#include <iostream>

#include "log.h"

namespace micrantha
{
    namespace prep
    {

        namespace internal {
            bool valid_term;
            bool init(const char *term) {
                static const std::string term_names[] = {"xterm", "ansi", "linux", "vt100"};
                if (term == nullptr) {
                    return false;
                }
                log_trace("found %s terminal", term);
                std::string test(term);
                for(const auto &term : term_names) {
                    if (term.find("xterm") != std::string::npos) {
                        return true;
                    }
                }
                return false;
            }
        }
        namespace color {
            std::string colorize(const std::vector<Value> &colors, const std::string &value, bool reset) {

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

        namespace vt100 {

            namespace cursor {
                void set(int rows, int cols) {
                    if (internal::valid_term) {
                        std::cout << "\033[" << rows << ";" << cols << "H" << std::flush;
                    }
                }
            }

            void init() {
                auto term = std::getenv("TERM");
                internal::valid_term = term != nullptr && internal::init(term);

                if (internal::valid_term) {
                    cursor::set(99999, 0);
                    std::cout << cursor::SAVE << std::flush;
                }
            }

            Progress::Progress() noexcept : alive_(true), bg_(std::bind(&Progress::run, this)) {

            }

            Progress::~Progress() {
                alive_ = false;
                if (bg_.joinable()) {
                    bg_.join();
                }
                reset();
            }

            void Progress::reset() {
                if (internal::valid_term) {
                    std::cout << cursor::RESTORE << std::flush;
                    std::cout << cursor::ERASE_LINE << std::flush;
                }
            }

            void Progress::run() {
                using namespace std::chrono_literals;
                while(internal::valid_term && alive_) {
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
                    std::cout << cursor::RESTORE << std::flush;
                    std::cout << color::colorize({color::attr::BOLD, color::fg::GREEN}, FRAMES[frame], true)
                              << cursor::BACK << std::flush;
                }
            }
        }
    }
}