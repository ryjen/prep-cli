
#include "log.h"

#include <cstdarg>
#include <fstream>

namespace micrantha {
    namespace prep {

        namespace log {

            namespace level {

                const char *NAMES[] = {"UNKN", "ERROR", "WARN", "INFO", "DEBUG", "TRACE", nullptr};

                const char *COLORS[] = {"", color::red, color::yellow, color::green, color::cyan, color::white, nullptr};

                Type __current_log_level = Info;

                bool valid(Type value) {
                    return (value <= __current_log_level);
                }

                bool set(const char *name) {
                    int i = 0;

                    if (name == nullptr || *name == 0) {
                        return false;
                    }

                    for (; NAMES[i] != nullptr; i++) {
                        if (!strcasecmp(name, NAMES[i])) {
                            __current_log_level = (Type) i;
                            return true;
                        }
                    }

                    return false;
                }

                std::string format(Type level) {
                    std::string buf("  ");

                    buf += COLORS[level];
                    buf += NAMES[level];
                    buf += color::clear;

                    buf += ": ";

                    return buf;
                }
            }
        }
    }
}
