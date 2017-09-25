#ifndef MICRANTHA_PREP_LOG_H
#define MICRANTHA_PREP_LOG_H

#include <cstring>

#ifndef __attribute__
#define __attribute__(x)
#endif

namespace micrantha
{
    namespace prep
    {
        typedef enum {
            /*! logging is disabled */
            LogNone = 0,
            /*! only error messages will be logged */
            LogError = 1,
            /*! warnings and errors will be logged */
            LogWarn = 2,
            /*! info, warning, and error messages will be logged */
            LogInfo = 3,
            /*! debug, info, warning and error messages will be logged */
            LogDebug = 4,
            /*! trace, debug, info, warning and error messages will be logged */
            LogTrace = 5
        } LogLevel;

        void set_log_level(const char *name);

        /*
         * TODO: remove usage of C-style variadic args
         */

        void log_error(const char *format, ...) __attribute__((format(printf, 1, 2)));

        void log_errno(int errnum);

        void log_warn(const char *format, ...) __attribute__((format(printf, 1, 2)));

        void log_info(const char *format, ...) __attribute__((format(printf, 1, 2)));

        void log_debug(const char *format, ...) __attribute__((format(printf, 1, 2)));

        void log_trace(const char *format, ...) __attribute__((format(printf, 1, 2)));
    }
}

#endif
