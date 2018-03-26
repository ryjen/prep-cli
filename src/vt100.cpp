//
// Created by Ryan Jennings on 2017-10-07.
//

#include "vt100.h"

#include <sys/termios.h>
#include <unistd.h>
#include <set>
#include <sstream>

#include "common.h"
#include "log.h"

namespace micrantha
{
    namespace prep
    {
        namespace internal
        {
            // indicates good terminal support
            bool valid_term;
            bool advanced_term;

            // size of the terminal
            // TODO: window resize
            int rows = 0, cols = 0;

            // tests a TERM value for validity
            static bool init(const char *term)
            {
                static const std::string Types[] = {"xterm", "ansi", "linux", "vt100"};

                if (term == nullptr) {
                    return false;
                }

                log::trace("found ", term, " terminal");

                std::string test(term);
                for (const auto &term : Types) {
                    if (test.find(term) != std::string::npos) {
                        return true;
                    }
                }
                return false;
            }
        }

        // color related functions
        namespace color
        {
            // colorizes a string
            std::string colorize(const std::vector<Value> &colors, const std::string &value, bool reset)
            {
                // don't colorize if unknown term
                if (!internal::valid_term) {
                    return value;
                }

                std::ostringstream buf;

                buf << "\033[";

                for (auto it = colors.begin(); it != colors.end(); ++it) {
                    buf << *it;
                    if (it != --colors.end()) {
                        buf << ";";
                    }
                }
                buf << "m" << value;

                if (reset) {
                    buf << color::CLEAR;
                }
                return buf.str();
            }

            std::string B(const std::string &value)
            {
                return colorize({fg::BLACK}, value, true);
            }

            std::string r(const std::string &value)
            {
                return colorize({fg::RED}, value, true);
            }

            std::string g(const std::string &value)
            {
                return colorize({fg::GREEN}, value, true);
            }

            std::string y(const std::string &value)
            {
                return colorize({fg::YELLOW}, value, true);
            }

            std::string b(const std::string &value)
            {
                return colorize({fg::BLUE}, value, true);
            }

            std::string m(const std::string &value)
            {
                return colorize({fg::MAGENTA}, value, true);
            }

            std::string c(const std::string &value)
            {
                return colorize({fg::CYAN}, value, true);
            }

            std::string w(const std::string &value)
            {
                return colorize({fg::WHITE}, value, true);
            }

            std::string apply(const std::string &color, const std::string &value, bool reset) {
                std::map<std::string, std::vector<Value>> mapping = {
                        {"r", {attr::NORMAL, fg::RED}},
                        {"g", {attr::NORMAL, fg::GREEN}},
                        {"y", {attr::NORMAL, fg::YELLOW}},
                        {"b", {attr::NORMAL, fg::BLUE}},
                        {"m", {attr::NORMAL, fg::MAGENTA}},
                        {"c", {attr::NORMAL, fg::CYAN}},
                        {"w", {attr::NORMAL, fg::WHITE}},
                        {"R", {attr::BOLD, fg::RED}},
                        {"G", {attr::BOLD, fg::GREEN}},
                        {"Y", {attr::BOLD, fg::YELLOW}},
                        {"B", {attr::BOLD, fg::BLUE}},
                        {"M", {attr::BOLD, fg::MAGENTA}},
                        {"C", {attr::BOLD, fg::CYAN}},
                        {"W", {attr::BOLD, fg::WHITE}}
                };

                auto it = mapping.find(color);

                if (it == mapping.end()) {
                    return value;
                }

                return colorize(it->second, value, reset);
            }
        }

        // other terminal codes
        namespace vt100
        {
            namespace internal
            {
                using namespace prep::internal;

                // reads a cursor report response from the terminal
                static ssize_t read_cursor(int fd, std::string &buf)
                {
                    static const std::set<char> Valids = {'\033', '[', ';', 'R'};
                    ssize_t numRead; /* # of bytes fetched by last read() */
                    size_t totRead;  /* Total bytes read so far */
                    char ch;

                    totRead = 0;

                    buf.clear();

                    for (;;) {
                        numRead = read(fd, &ch, 1);

                        if (numRead == -1) {
                            if (errno == EINTR) /* Interrupted --> restart read() */
                                continue;
                            else
                                return -1; /* Some other error */

                        } else if (numRead == 0) { /* EOF */
                            if (totRead == 0)      /* No bytes read; return 0 */
                                return 0;
                            else /* Some bytes read; add '\0' */
                                break;

                        } else { /* 'numRead' must be 1 if we get here */
                            totRead++;

                            if (ch != '\n' && ch != '\r' && (Valids.find(ch) != Valids.end() || std::isdigit(ch))) {
                                buf += ch;
                            }

                            if (ch == 'R')
                                break;
                        }
                    }

                    return totRead;
                }

                /**
                 * RAII class to temporarily disable echo mode so we can read a response from the terminal
                 */
                class TTYRead
                {
                   public:
                    TTYRead() : state_(), saved_()
                    {
                        // get state
                        tcgetattr(STDIN_FILENO, &state_);
                        // copy state
                        saved_ = state_;

                        // turn off canonical mode and echo
                        state_.c_lflag &= ~(ICANON | ECHO);
                        // minimum of number input read.
                        state_.c_cc[VMIN] = 1;
                        // timeout
                        state_.c_cc[VTIME] = 2;

                        // set the new terminal attributes.
                        tcsetattr(STDIN_FILENO, TCSANOW, &state_);
                    }

                    ~TTYRead()
                    {
                        // reset state
                        tcsetattr(STDIN_FILENO, TCSANOW, &saved_);
                    }

                    /**
                     * @param row the cursor row to find
                     * @param col the cursor col to find
                     */
                    int read_cursor(int *row, int *col) const
                    {
                        std::string line;

                        // request the cursor position
                        print(cursor::REPORT) << std::flush;

                        // read the response
                        if (internal::read_cursor(STDIN_FILENO, line) < 0) {
                            log::perror(errno);
                            return PREP_FAILURE;
                        }

                        // parse the response
                        if (sscanf(line.c_str(), "\033[%d;%dR", row, col) != 2) {
                            log::error("unable to parse cursor position [", line, "]");
                            return PREP_FAILURE;
                        }

                        return PREP_SUCCESS;
                    }

                   private:
                    struct termios state_, saved_;
                };
            }

            // output related functions
            namespace output
            {
                // utility for variadic print
                std::ostream &print(std::ostream &os)
                {
                    return os;
                }

                // output functions have their own mutex
                Mutex &get_mutex()
                {
                    static Mutex m;
                    return m;
                }
            }
            namespace cursor
            {
                Mutex &get_mutex()
                {
                    static Mutex m;
                    return m;
                }

                int set(int rows, int cols)
                {
                    if (internal::advanced_term && rows > 0 && cols > 0) {
                        print("\033[", rows, ";", cols, "H") << std::flush;
                        return PREP_SUCCESS;
                    }
                    return PREP_FAILURE;
                }

                int get(int *row, int *col)
                {
                    if (!internal::advanced_term) {
                        return PREP_FAILURE;
                    }

                    // assert we can read
                    internal::TTYRead ttyRead;

                    return ttyRead.read_cursor(row, col);
                }
            }

            void init(bool simple)
            {
                if (log::output::is_file()) {
                    return;
                }

                // get the type of terminal
                auto term = std::getenv("TERM");

                // test validity
                internal::valid_term = term != nullptr && internal::init(term);

                internal::advanced_term = !simple && internal::valid_term;

                if (internal::advanced_term) {

                    // find the screen height by setting the cursor to a extreme value
                    cursor::set(99999, 99999);

                    // get the actual position
                    cursor::get(&internal::rows, &internal::cols);
                }
            }

            void disable_user()
            {
                struct termios state = {};

                // get state
                tcgetattr(STDIN_FILENO, &state);

                // turn off canonical mode and echo
                state.c_lflag &= ~(ICANON | ECHO);
                // minimum of number input read.
                state.c_cc[VMIN] = 1;
                // timeout
                state.c_cc[VTIME] = 2;

                // set the new terminal attributes.
                tcsetattr(STDIN_FILENO, TCSANOW, &state);
            }
        }
    }
}