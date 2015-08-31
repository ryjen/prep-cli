#include "config.h"
#include "environment.h"
#include "util.h"
#include "common.h"
#include "log.h"
#include "repository.h"
#include <sstream>

using namespace std;

namespace arg3
{
	namespace prep
	{

		string environment::build_ldflags(const string & varName) const
		{
			ostringstream buf;
			const char *temp = repository::get_local_repo();

			if (directory_exists(temp)) {
				buf << "-L" << temp << "/lib ";
			}

			if (directory_exists(repository::GLOBAL_REPO)) {
				buf << "-L" << repository::GLOBAL_REPO << "/lib ";
			}

			temp = getenv(varName.c_str());

			if (temp) {
				buf << temp;
			}

			string flags = buf.str();

			if (flags.empty()) {
				return flags;
			}

			return varName + "=" + flags;
		}

		string environment::build_cflags(const string & varName) const
		{
			ostringstream buf;
			const char *temp = repository::get_local_repo();

			if (directory_exists(temp)) {
				buf << "-I" << temp << "/include ";
			}

			if (directory_exists(repository::GLOBAL_REPO)) {
				buf << "-I" << repository::GLOBAL_REPO << "/include ";
			}

			temp = getenv(varName.c_str());

			if (temp) {
				buf << temp;
			}

			string flags = buf.str();

			if (flags.empty()) {
				return flags;
			}

			return varName + "=" + flags;
		}

		string environment::build_path() const
		{
			ostringstream buf;
			const char *temp = repository::get_local_repo();

			if (directory_exists(temp)) {
				buf << temp << "/bin:";
			}

			if (directory_exists(repository::GLOBAL_REPO)) {
				buf << repository::GLOBAL_REPO << "/bin:";
			}

			temp = getenv("PATH");

			if (temp) {
				buf << temp;
			}

			string flags = buf.str();

			if (flags.empty()) {
				return flags;
			}

			return "PATH=" + flags;
		}

		string environment::build_ldpath() const
		{
			ostringstream buf;
			const char *temp = repository::get_local_repo();

			if (directory_exists(temp)) {
				buf << temp << "/lib:";
			}

			if (directory_exists(repository::GLOBAL_REPO)) {
				buf << repository::GLOBAL_REPO << "/lib:";
			}

#ifdef __APPLE__
			temp = getenv("DYLD_LIBRARY_PATH");
#else
			temp = getenv("LD_LIBRARY_PATH");
#endif

			if (temp) {
				buf << temp;
			}

			string flags = buf.str();

			if (flags.empty()) {
				return flags;
			}
#ifdef __APPLE__
			return "DYLD_LIBRARY_PATH=" + flags;
#else
			return "LD_LIBRARY_PATH=" + flags;
#endif
		}

		int environment::execute(const char *command, const char *path) const
		{
			char flags[5][BUFSIZ];
			const char *args[] = { "/bin/sh", "-c", command, NULL };

			strncpy(flags[0], build_cflags("CPPFLAGS").c_str(), BUFSIZ);
			strncpy(flags[1], build_cflags("CXXFLAGS").c_str(), BUFSIZ);
			strncpy(flags[2], build_ldflags("LDFLAGS").c_str(), BUFSIZ);
			strncpy(flags[3], build_path().c_str(), BUFSIZ);
			strncpy(flags[4], build_ldpath().c_str(), BUFSIZ);

			char *const envp[] = {
				flags[0], flags[1], flags[2], flags[3], flags[4], NULL
			};

			if (fork_command(args, path, envp))
			{
				log_error("unable to execute %s", command);
				return PREP_FAILURE;
			}

			return PREP_SUCCESS;
		}

	}
}