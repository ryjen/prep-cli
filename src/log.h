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

            using namespace level;

            template<class ...Args>
            void info(const Args &...args) {
                if (!valid(Info)) {
                    return;
                }
                io::println(format(Info), args...) << std::flush;
            }

            template<class ...Args>
            void debug(const Args &...args) {
                if (!valid(Debug)) {
                    return;
                }
                io::println(format(Debug), args...) << std::flush;
            }

            template<class ...Args>
            void error(const Args &...args) {
                if (!valid(Error)) {
                    return;
                }
                io::println(format(Error), args...) << std::flush;
            }

            template<class ...Args>
            void warn(const Args &...args) {
                if (!valid(Warn)) {
                    return;
                }
                io::println(format(Warn), args...) << std::flush;
            }

            template<class ...Args>
            void trace(const Args &...args) {
                if (!valid(Trace)) {
                    return;
                }
                io::println(format(Trace), args...) << std::flush;
            }

            template<class ...Args>
            void perror(const Args &...args) {
                error(std::to_string(errno), " - ", strerror(errno), " - ", args...);
            }
        }
    }
}

#endif
