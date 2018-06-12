//
// Created by Ryan Jennings on 2017-10-07.
//

#ifndef MICRANTHA_PREP_VT100_H
#define MICRANTHA_PREP_VT100_H

#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "util.h"

namespace micrantha {
    namespace prep {
        // do some fun stuff with output
        namespace vt100 {
            /**
             * tests if the stdout has a tty
             * @return true if a valid tty
             */
            bool is_valid_term();
        }

        // color related utils
        namespace color {
            typedef unsigned Value;

            // foreground colors
            namespace fg {
                typedef enum {
                    BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE
                } Type;
            }

            // background colors
            namespace bg {
                typedef enum {
                    BLACK = 40, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE
                } Type;
            }

            // color attributes
            namespace attr {
                typedef enum {
                    NORMAL, BOLD
                } Type;
            }

            using Foreground = fg::Type;
            using Background = bg::Type;
            using Attribute = attr::Type;


            /**
             * clears previous color codes
             */
            constexpr static const char *const clear = "\033[0m";

            constexpr static const char *const dark = "\033[0;30m";
            constexpr static const char *const Dark = "\033[1;30m";
            constexpr static const char *const red = "\033[0;31m";
            constexpr static const char *const Red = "\033[1;31m";
            constexpr static const char *const green = "\033[0;32m";
            constexpr static const char *const Green = "\033[1;32m";
            constexpr static const char *const yellow = "\033[0;33m";
            constexpr static const char *const Yellow = "\033[1;33m";
            constexpr static const char *const blue = "\033[0;34m";
            constexpr static const char *const Blue = "\033[1;34m";
            constexpr static const char *const magenta = "\033[0;35m";
            constexpr static const char *const Magenta = "\033[1;35m";
            constexpr static const char *const cyan = "\033[0;36m";
            constexpr static const char *const Cyan = "\033[1;36m";
            constexpr static const char *const white = "\033[0;37m";
            constexpr static const char *const White = "\033[1;37m";

            /**
             * colorizes a string
             * @param colors the colors to set
             * @param value the string to display
             * @param reset true if the colors should reset after the value is displayed
             * @return a string with vt100 color information
             */
            std::string colorize(const std::vector<Value> &colors, const std::string &value, bool reset = true);

            /**
             * parse a string into a color:
             * Can be r,g,b,y,c,w
             * @param value
             * @return the color value
             */
            std::string apply(const std::string &color, const std::string &value, bool reset = true);

            /**
             * apply BLACK to text
             * @param value the text
             * @return the text wrapped in BLACK
             */
            std::string d(const std::string &value);

            std::string D(const std::string &value);

            /**
             * apply RED to text
             * @param value the text
             * @return the text wrapped in RED
             */
            std::string r(const std::string &value);

            std::string R(const std::string &value);

            /**
             * apply GREEN to a value
             * @param value the text
             * @return the text wrapped in GREEN
             */
            std::string g(const std::string &value);

            std::string G(const std::string &value);

            /**
             * apply YELLOW to a value
             * @param value the text
             * @return the text wrapped in YELLOW
             */
            std::string y(const std::string &value);

            std::string Y(const std::string &value);

            /**
             * apply BLUE to a value
             * @param value the text
             * @return the text wrapped in BLUE
             */
            std::string b(const std::string &value);

            std::string B(const std::string &value);

            /**
             * apply MAGENTA to a value
             * @param value the text
             * @return the text wrapped in MAGENTA
             */
            std::string m(const std::string &value);

            std::string M(const std::string &value);

            /**
             * apply CYAN to a value
             * @param value the text
             * @return the text wrapped in CYAN
             */
            std::string c(const std::string &value);

            std::string C(const std::string &value);

            /**
             * apply WHITE to a value
             * @param value the text
             * @return the text wrapped in WHITE
             */
            std::string w(const std::string &value);

            std::string W(const std::string &value);

        }
    }
}

#endif  // PREP_VT100_H
