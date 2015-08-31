#ifndef INCLUDE_ENVIRONMENT_H
#define INCLUDE_ENVIRONMENT_H

#include <string>

namespace arg3
{
	namespace prep
	{
		class environment
		{

		public:
			std::string build_cflags(const std::string &varName) const;
			std::string build_ldflags(const std::string &varName) const;
			std::string build_path() const;
			std::string build_ldpath() const;

			int execute(const char *command, const char *path) const;

		private:

		};
	}
}

#endif
