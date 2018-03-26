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

namespace micrantha
{
    namespace prep
    {
        // do some fun stuff with output
        namespace vt100
        {
            // define a mutex type
            typedef std::recursive_mutex Mutex;

            // output related functions
            namespace output
            {
                /**
                 * utility method for variadic print
                 * @param os the output stream
                 * @return the output stream
                 */
                std::ostream &print(std::ostream &os);

                /**
                 * variadic print
                 * @tparam A0 the type of argument
                 * @tparam Args the remaining arguments
                 * @param os the output stream
                 * @param a0 the argument
                 * @param args the remaining arguments
                 * @return
                 */
                template <class A0, class... Args>
                std::ostream &print(std::ostream &os, const A0 &a0, const Args &... args)
                {
                    // print the argument
                    os << a0;
                    // print the remaining arguments
                    return print(os, args...);
                }

                /**
                 * variadic print
                 * @tparam Args the arguments
                 * @param os the output stream
                 * @param args the arguments
                 * @return the output stream
                 */
                template <class... Args>
                std::ostream &print(std::ostream &os, const Args &... args)
                {
                    // pass the first argument to the printer
                    return print(os, args...);
                }

                /**
                 * output methods have their own mutex
                 * @return a single instance mutex
                 */
                Mutex &get_mutex();
            }

            /**
             * variadic print
             * @tparam Args the type of arguments
             * @param args the arguments
             * @return the output stream
             */
            template <class... Args>
            std::ostream &print(const Args &... args)
            {
                std::lock_guard<Mutex> _(output::get_mutex());
                return output::print(std::cout, args...);
            }

            // cursor related
            namespace cursor
            {
                /**
                 * saves a cursor position and attributes
                 */
                constexpr static const char *const SAVE = "\0337";
                /**
                 * restores a saved cursor position and attributes
                 */
                constexpr static const char *const RESTORE = "\0338";

                /**
                 * erases from cursor to the start of the line
                 */
                constexpr static const char *const ERASE_BACK = "\033[1K";

                /**
                 * asks the terminal to report the cursor position for reading
                 */
                constexpr static const char *const REPORT = "\033[6n";

                /**
                 * sets the cursor position
                 * @param rows
                 * @param cols
                 * @return PREP_SUCCESS or PREP_FAILURE
                 */
                int set(int rows, int cols);

                /**
                 * gets the cursor position
                 * NOTE: may be blocking
                 * @param rows
                 * @param cols
                 * @return PREP_SUCCESS or PREP_FAILURE
                 */
                int get(int &rows, int &cols);
            }

            /**
             * initialize the terminal
             */
            void init(bool simple);

            /**
             * turn off user input
             */
            void disable_user();
        }

        // color related utils
        namespace color
        {
            typedef unsigned Value;

            // foreground colors
            namespace fg
            {
                typedef enum { BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE } Type;
            }

            // background colors
            namespace bg
            {
                typedef enum { BLACK = 40, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE } Type;
            }

            // color attributes
            namespace attr
            {
                typedef enum { NORMAL, BOLD } Type;
            }

            using Foreground = fg::Type;
            using Background = bg::Type;
            using Attribute = attr::Type;

            /**
             * clears previous color codes
             */
            constexpr static const char *const CLEAR = "\033[0m";

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
            std::string B(const std::string &value);

            /**
             * apply RED to text
             * @param value the text
             * @return the text wrapped in RED
             */
            std::string r(const std::string &value);

            /**
             * apply GREEN to a value
             * @param value the text
             * @return the text wrapped in GREEN
             */
            std::string g(const std::string &value);

            /**
             * apply YELLOW to a value
             * @param value the text
             * @return the text wrapped in YELLOW
             */
            std::string y(const std::string &value);

            /**
             * apply BLUE to a value
             * @param value the text
             * @return the text wrapped in BLUE
             */
            std::string b(const std::string &value);

            /**
             * apply MAGENTA to a value
             * @param value the text
             * @return the text wrapped in MAGENTA
             */
            std::string m(const std::string &value);

            /**
             * apply CYAN to a value
             * @param value the text
             * @return the text wrapped in CYAN
             */
            std::string c(const std::string &value);

            /**
             * apply WHITE to a value
             * @param value the text
             * @return the text wrapped in WHITE
             */
            std::string w(const std::string &value);

        }
    }
}

#endif  // PREP_VT100_H
