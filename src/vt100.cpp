//
// Created by Ryan Jennings on 2017-10-07.
//

#include "vt100.h"

#include <sys/termios.h>
#include <unistd.h>
#include <set>
#include <sstream>

#include "common.h"
#include "log.h"

namespace micrantha
{
    namespace prep
    {
        namespace internal
        {
            // indicates good terminal support
            bool valid_term;

            // tests a TERM value for validity
            static bool init(const char *term)
            {
                static const std::string Types[] = {"xterm", "ansi", "linux", "vt100"};

                if (term == nullptr) {
                    return false;
                }

                log::trace("found ", term, " terminal");

                std::string test(term);
                for (const auto &term : Types) {
                    if (test.find(term) != std::string::npos) {
                        return true;
                    }
                }
                return false;
            }
        }

        // color related functions
        namespace color
        {
            // colorizes a string
            std::string colorize(const std::vector<Value> &colors, const std::string &value, bool reset)
            {
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

            std::string B(const std::string &value)
            {
                return colorize({fg::BLACK}, value, true);
            }

            std::string r(const std::string &value)
            {
                return colorize({fg::RED}, value, true);
            }

            std::string g(const std::string &value)
            {
                return colorize({fg::GREEN}, value, true);
            }

            std::string y(const std::string &value)
            {
                return colorize({fg::YELLOW}, value, true);
            }

            std::string b(const std::string &value)
            {
                return colorize({fg::BLUE}, value, true);
            }

            std::string m(const std::string &value)
            {
                return colorize({fg::MAGENTA}, value, true);
            }

            std::string c(const std::string &value)
            {
                return colorize({fg::CYAN}, value, true);
            }

            std::string w(const std::string &value)
            {
                return colorize({fg::WHITE}, value, true);
            }

            std::string apply(const std::string &color, const std::string &value, bool reset) {
                std::map<std::string, std::vector<Value>> mapping = {
                        {"r", {attr::NORMAL, fg::RED}},
                        {"g", {attr::NORMAL, fg::GREEN}},
                        {"y", {attr::NORMAL, fg::YELLOW}},
                        {"b", {attr::NORMAL, fg::BLUE}},
                        {"m", {attr::NORMAL, fg::MAGENTA}},
                        {"c", {attr::NORMAL, fg::CYAN}},
                        {"w", {attr::NORMAL, fg::WHITE}},
                        {"R", {attr::BOLD, fg::RED}},
                        {"G", {attr::BOLD, fg::GREEN}},
                        {"Y", {attr::BOLD, fg::YELLOW}},
                        {"B", {attr::BOLD, fg::BLUE}},
                        {"M", {attr::BOLD, fg::MAGENTA}},
                        {"C", {attr::BOLD, fg::CYAN}},
                        {"W", {attr::BOLD, fg::WHITE}}
                };

                auto it = mapping.find(color);

                if (it == mapping.end()) {
                    return value;
                }

                return colorize(it->second, value, reset);
            }
        }

        // other terminal codes
        namespace vt100
        {

            // output related functions
            namespace output
            {
                // utility for variadic print
                std::ostream &print(std::ostream &os)
                {
                    return os;
                }

                // output functions have their own mutex
                Mutex &get_mutex()
                {
                    static Mutex m;
                    return m;
                }
            }

            void init(bool simple)
            {
                if (log::output::is_file()) {
                    return;
                }

                // get the type of terminal
                auto term = std::getenv("TERM");

                // test validity
                internal::valid_term = term != nullptr && internal::init(term);
            }
        }
    }
}