#ifndef RJ_PREP_LOG_H
#define RJ_PREP_LOG_H

#include <string.h>

#ifndef __attribute__
#define __attribute__(x)
#endif

namespace rj
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

        void log_error(const char *const format, ...) __attribute__((format(printf, 1, 2)));

        void log_warn(const char *const format, ...) __attribute__((format(printf, 1, 2)));

        void log_info(const char *const format, ...) __attribute__((format(printf, 1, 2)));

        void log_debug(const char *const format, ...) __attribute__((format(printf, 1, 2)));

        void log_trace(const char *const format, ...) __attribute__((format(printf, 1, 2)));
    }
}

// A macro so the correct method name is displayed
#define log_errno(errnum) log_error("%d: %s", errnum, strerror(errnum))

#endif
