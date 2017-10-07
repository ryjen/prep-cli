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

                std::string format(level::Type value);
            }

            using namespace level;
            namespace callback = vt100::output;

            template<class ...Args>
            void info(const Args &...args) {
                if (!valid(Info)) {
                    return;
                }
                vt100::print(format(Info), args..., "\n");
                callback::on_newline();
            }

            template<class ...Args>
            void debug(const Args &...args) {
                if (!valid(Debug)) {
                    return;
                }
                vt100::print(format(Debug), args..., "\n");
                callback::on_newline();
            }

            template<class ...Args>
            void error(const Args &...args) {
                if (!valid(Error)) {
                    return;
                }
                vt100::print(format(Error), args..., "\n");
                callback::on_newline();
            }

            template<class ...Args>
            void warn(const Args &...args) {
                if (!valid(Warn)) {
                    return;
                }
                vt100::print(format(Warn), args..., "\n");
                callback::on_newline();
            }

            template<class ...Args>
            void trace(const Args &...args) {
                if (!valid(Trace)) {
                    return;
                }
                vt100::print(format(Trace), args..., "\n");
                callback::on_newline();
            }

            template<class ...Args>
            void perror(const Args &...args) {
                error(std::to_string(errno), ": ", strerror(errno));
            }
        }
    }
}

#endif
