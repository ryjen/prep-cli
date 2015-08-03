#ifndef PREP_LOG_H_
#define PREP_LOG_H_

#include <string.h>

#ifndef __attribute__
#define __attribute__(x)
#endif

namespace arg3
{
	namespace prep
	{
		typedef enum
		{
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

#define log_errno(errnum)  log_error("%d: %s", errnum, strerror(errnum))

#endif
