#ifndef MICRANTHA_PREP_LOG_H
#define MICRANTHA_PREP_LOG_H

#include <cstring>
#include <ostream>
#include <cerrno>

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

                void set(const char *name);

                bool valid(Type value);

                std::ostream &format(level::Type value);
            }

            namespace output {
                std::ostream &print(std::ostream &os);

                template<class A0, class ...Args>
                std::ostream &print(std::ostream &os, const A0 &a0, const Args &...args) {
                    os << a0;
                    return print(os, args...);
                }

                template<class ...Args>
                std::ostream &print(std::ostream &os, const Args &...args) {
                    return print(os, args...);
                }
            }

            using namespace level;

            template<class ...Args>
            void info(const Args &...args) {
                if (!valid(Info)) {
                    return;
                }
                std::lock_guard<std::mutex> _(vt100::output::get_mutex());
                output::print(format(Info), args..., "\n");
            }

            template<class ...Args>
            void debug(const Args &...args) {
                if (!valid(Debug)) {
                    return;
                }
                std::lock_guard<std::mutex> _(vt100::output::get_mutex());
                output::print(format(Debug), args..., "\n");
            }

            template<class ...Args>
            void error(const Args &...args) {
                if (!valid(Error)) {
                    return;
                }
                std::lock_guard<std::mutex> _(vt100::output::get_mutex());
                output::print(format(Error), args..., "\n");
            }

            template<class ...Args>
            void warn(const Args &...args) {
                if (!valid(Warn)) {
                    return;
                }
                std::lock_guard<std::mutex> _(vt100::output::get_mutex());
                output::print(format(Warn), args..., "\n");
            }

            template<class ...Args>
            void trace(const Args &...args) {
                if (!valid(Trace)) {
                    return;
                }
                std::lock_guard<std::mutex> _(vt100::output::get_mutex());
                output::print(format(Trace), args..., "\n");
            }

            template<class ...Args>
            void perror(const Args &...args) {
                error(std::to_string(errno), ": ", strerror(errno));
            }
        }
    }
}

#endif
