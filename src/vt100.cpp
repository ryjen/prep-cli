//
// Created by Ryan Jennings on 2017-10-07.
//

#include "vt100.h"

#include <termios.h>
#include <unistd.h>
#include <set>
#include <sstream>

#include "common.h"
#include "log.h"

namespace micrantha
{
    namespace prep
    {
        // other terminal codes
        namespace vt100
        {
            bool is_valid_term()
            {
                static bool value = isatty(STDOUT_FILENO) == 1;
                return value;
            }
        }

        // color related functions
        namespace color
        {
            // colorizes a string
            std::string colorize(const std::vector<Value> &colors, const std::string &value, bool reset)
            {
                // don't colorize if unknown term
                if (!vt100::is_valid_term()) {
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
                    buf << color::clear;
                }
                return buf.str();
            }

            std::string d(const std::string &value)
            {
                return colorize({attr::NORMAL, fg::BLACK}, value, true);
            }
            std::string D(const std::string &value)
            {
                return colorize({attr::BOLD, fg::BLACK}, value, true);
            }
            std::string r(const std::string &value)
            {
                return colorize({attr::NORMAL, fg::RED}, value, true);
            }
            std::string R(const std::string &value)
            {
                return colorize({attr::BOLD, fg::RED}, value, true);
            }
            std::string g(const std::string &value)
            {
                return colorize({attr::NORMAL,fg::GREEN}, value, true);
            }
            std::string G(const std::string &value)
            {
                return colorize({attr::BOLD, fg::GREEN}, value, true);
            }
            std::string y(const std::string &value)
            {
                return colorize({attr::NORMAL, fg::YELLOW}, value, true);
            }
            std::string Y(const std::string &value)
            {
                return colorize({attr::BOLD, fg::YELLOW}, value, true);
            }
            std::string b(const std::string &value)
            {
                return colorize({attr::NORMAL, fg::BLUE}, value, true);
            }
            std::string B(const std::string &value)
            {
                return colorize({attr::BOLD, fg::BLUE}, value, true);
            }
            std::string m(const std::string &value)
            {
                return colorize({attr::NORMAL, fg::MAGENTA}, value, true);
            }
            std::string M(const std::string &value)
            {
                return colorize({attr::BOLD, fg::MAGENTA}, value, true);
            }
            std::string c(const std::string &value)
            {
                return colorize({attr::NORMAL, fg::CYAN}, value, true);
            }
            std::string C(const std::string &value)
            {
                return colorize({attr::BOLD, fg::CYAN}, value, true);
            }
            std::string w(const std::string &value)
            {
                return colorize({attr::NORMAL, fg::WHITE}, value, true);
            }
            std::string W(const std::string &value)
            {
                return colorize({attr::BOLD, fg::WHITE}, value, true);
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

    }
}