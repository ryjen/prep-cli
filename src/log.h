#ifndef MICRANTHA_PREP_LOG_H
#define MICRANTHA_PREP_LOG_H

#include <cstring>
#include <ostream>
#include <cerrno>
#include <iostream>

#include "vt100.h"

namespace micrantha {
    namespace prep {
        namespace log {

            namespace level {
                typedef enum {
                    /*! logging is disabled */
                            None = 0,
                    /*! only error messages will be logged */
                            Error = 1,
                    /*! warnings and errors will be logged */
                            Warn = 2,
                    /*! info, warning, and error messages will be logged */
                            Info = 3,
                    /*! debug, info, warning and error messages will be logged */
                            Debug = 4,
                    /*! trace, debug, info, warning and error messages will be logged */
                            Trace = 5
                } Type;

                bool set(const char *name);

                bool valid(Type value);

                std::string format(level::Type value);
            }

            namespace output {

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
                 * variadic print
                 * @tparam Args the type of arguments
                 * @param args the arguments
                 * @return the output stream
                 */
                template <class... Args>
                std::ostream &print(const Args &... args)
                {
                    return print(std::cout, args...);
                }
            }

            using namespace level;

            template<class ...Args>
            void info(const Args &...args) {
                if (!valid(Info)) {
                    return;
                }
                output::print(format(Info), args..., "\n") << std::flush;
            }

            template<class ...Args>
            void debug(const Args &...args) {
                if (!valid(Debug)) {
                    return;
                }
                output::print(format(Debug), args..., "\n") << std::flush;
            }

            template<class ...Args>
            void error(const Args &...args) {
                if (!valid(Error)) {
                    return;
                }
                output::print(format(Error), args..., "\n") << std::flush;
            }

            template<class ...Args>
            void warn(const Args &...args) {
                if (!valid(Warn)) {
                    return;
                }
                output::print(format(Warn), args..., "\n") << std::flush;
            }

            template<class ...Args>
            void trace(const Args &...args) {
                if (!valid(Trace)) {
                    return;
                }
                output::print(format(Trace), args..., "\n") << std::flush;
            }

            template<class ...Args>
            void perror(const Args &...args) {
                error(std::to_string(errno), ": ", strerror(errno), " - ", args...);
            }
        }
    }
}

#endif
