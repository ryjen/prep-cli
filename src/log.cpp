
#include "log.h"

#include <cstdarg>
#include <fstream>

namespace micrantha {
    namespace prep {

        namespace log {
            namespace output {
                std::ostream &print(std::ostream &os) {
                    return os;
                }
            }
            namespace level {

                const char *NAMES[] = {"UNKN", "ERROR", "WARN", "INFO", "DEBUG", "TRACE", nullptr};

                const char *COLORS[] = {"", "r", "y", "g", "c", "w", nullptr};

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

                    buf += color::apply(COLORS[level], NAMES[level]);

                    buf += ": ";

                    return buf;
                }
            }
        }
    }
}
