#include "config.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <execinfo.h>
#include <dlfcn.h>
#include "log.h"

namespace arg3
{

	namespace prep
	{
		const char *LOG_LEVEL_NAMES[] =
		{
			"UNKNOWN", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
		};

		LogLevel __current_log_level = LogInfo;

		static void log_vargs(LogLevel level, const char *const format, va_list args)
		{
			fprintf(stdout, "%s: ", LOG_LEVEL_NAMES[level]);
			vfprintf(stdout, format, args);
			fputs("\n", stdout);
			fflush(stdout);

		}

		void log_error(const char *const format, ...)
		{
			va_list args;

			if (LogError > __current_log_level) { return; }

			va_start(args, format);
			log_vargs(LogError, format, args);
			va_end(args);
		}

		void log_warn(const char *const format, ...)
		{
			va_list args;

			if (LogWarn > __current_log_level) { return; }

			va_start(args, format);
			log_vargs(LogWarn, format, args);
			va_end(args);
		}

		void log_info(const char *const format, ...)
		{
			va_list args;

			if (LogInfo > __current_log_level) { return; }

			va_start(args, format);
			log_vargs(LogInfo, format, args);
			va_end(args);
		}

		void log_debug(const char *const format, ...)
		{
			va_list args;

			if (LogDebug > __current_log_level) { return; }

			va_start(args, format);
			log_vargs(LogDebug, format, args);
			va_end(args);
		}

		void log_trace(const char *const format, ...)
		{
			va_list args;

			if (LogTrace > __current_log_level) { return; }

			va_start(args, format);
			log_vargs(LogTrace, format, args);
			va_end(args);
		}

	}
}


