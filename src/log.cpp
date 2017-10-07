
#include "log.h"

#include <cstdarg>

namespace micrantha {
    namespace prep {
        namespace internal {
            extern bool valid_term;
        }

        namespace log {
            namespace output {
                std::ostream &print(std::ostream &os) {
                    return os;
                }
            }
            namespace level {

                const char *NAMES[] = {"UNKN", "ERROR", "WARN", "INFO", "DEBUG", "TRACE", nullptr};

                const char *COLORS[] = {"", "\033[1;31m", "\033[1;33m", "\033[1;32m",
                                        "\033[1;36m", "\033[1;37m", nullptr};

                Type __current_log_level = Info;

                bool valid(Type value) {
                    return (value <= __current_log_level);
                }

                void set(const char *name) {
                    int i = 0;

                    if (name == nullptr || *name == 0) {
                        return;
                    }

                    for (; NAMES[i] != nullptr; i++) {
                        if (!strcasecmp(name, NAMES[i])) {
                            __current_log_level = (Type) i;
                        }
                    }
                }

                std::ostream &format(Type level) {
                    std::cout << "  ";

                    if (internal::valid_term) {
                        std::cout << COLORS[level];
                    }
                    std::cout << NAMES[level];
                    if (internal::valid_term) {
                        std::cout << color::CLEAR;
                    }
                    std::cout << ": ";
                    return std::cout;
                }
            }
        }
    }
}
