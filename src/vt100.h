//
// Created by Ryan Jennings on 2017-10-07.
//

#ifndef MICRANTHA_PREP_VT100_H
#define MICRANTHA_PREP_VT100_H

#include <thread>
#include <string>
#include <vector>
#include <iostream>

namespace micrantha {
    namespace prep {

        // do some fun stuff with output
        namespace vt100 {

            namespace output {
                std::ostream& print(std::ostream& os);

                template<class A0, class ...Args>
                std::ostream &
                print(std::ostream &os, const A0 &a0, const Args &...args) {
                    os << a0;
                    return print(os, args...);
                }

                template<class ...Args>
                std::ostream &
                print(std::ostream &os, const Args &...args) {
                    return print(os, args...);
                }

                std::mutex &get_mutex();
            }

            template <class ...Args>
            std::ostream&
            print(const Args& ...args)
            {
                std::lock_guard<std::mutex> _(output::get_mutex());
                return output::print(std::cout, args...);
            }

            // cursor related
            namespace cursor {
                constexpr static const char *const BACK = "\033[1D";
                constexpr static const char *const SAVE = "\0337";
                constexpr static const char *const RESTORE = "\0338";
                constexpr static const char *const ERASE = "\033[1K";
                constexpr static const char *const REPORT = "\033[6n";
                constexpr static const char *const UP = "\033[1A";
                constexpr static const char *const DOWN = "\033[1B";

                /**
                 * sets the cursor position
                 * @param rows
                 * @param cols
                 */
                void set(int rows, int cols);

                /**
                 * gets the cursor position
                 * NOTE: may be blocking
                 * @param rows
                 * @param cols
                 */
                void get(int &rows, int &cols);

                /**
                 * RAII class to save and restore a cursor in scope
                 */
                class Savepoint {
                public:
                    Savepoint();
                    ~Savepoint();
                    Savepoint(const Savepoint &) = delete;
                    Savepoint(Savepoint &&) = delete;
                    Savepoint &operator=(const Savepoint&) = delete;
                    Savepoint &operator=(Savepoint &&) = delete;
                };
            }

            /**
             * initialize the terminal
             */
            void init();

            /**
             * RAII class to display a progress indicator
             */
            class Progress {
            public:
                Progress() noexcept;

                ~Progress();

                Progress(const Progress &) = delete;

                Progress(Progress &&) = delete;

                Progress &operator=(const Progress &) = delete;

                Progress &operator=(Progress &&) = delete;

            private:
                void update();

                void reset();

                void run();

                bool alive_;
                int row_;
                std::thread bg_;
            };
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
            constexpr static const char *const CLEAR = "\033[0m";

            /**
             * colorizes a string
             * @param colors the colors to set
             * @param value the string to display
             * @param reset true if the colors should reset after the value is displayed
             * @return a string with vt100 color information
             */
            std::string colorize(const std::vector <Value> &colors, const std::string &value, bool reset = true);

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

#endif //PREP_VT100_H
