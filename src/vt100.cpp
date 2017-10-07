//
// Created by Ryan Jennings on 2017-10-07.
//

#include "vt100.h"

#include <sstream>
#include <sys/termios.h>
#include <unistd.h>
#include <set>

#include "log.h"

namespace micrantha {
    namespace prep {

        namespace internal {

            // indicates good terminal support
            bool valid_term;

            // size of the terminal
            // TODO: window resize
            int rows = 0, cols = 0;

            // tests a TERM value for validity
            bool init(const char *term) {
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


            // reads a cursor report response from the terminal
            ssize_t read_report(int fd, std::string &buf)
            {
                static const std::set<char> Valids = { '\033', '[', ';', 'R'};
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
             * RAII class to disable echo mode so we can read a response from the terminal
             */
            class TTYRead {
            public:
                TTYRead()
                {
                    // get state
                    tcgetattr(STDIN_FILENO, &state_);
                    // copy state
                    saved_ = state_;

                    //turn off canonical mode and echo
                    state_.c_lflag &= ~(ICANON | ECHO);
                    //minimum of number input read.
                    state_.c_cc[VMIN] = 1;
                    // timeout
                    state_.c_cc[VTIME] = 2;

                    //set the new terminal attributes.
                    tcsetattr(STDIN_FILENO, TCSANOW, &state_);

                }
                ~TTYRead() {
                    // reset state
                    tcsetattr(STDIN_FILENO, TCSANOW, &saved_);
                }
            private:
                struct termios state_, saved_;
            };

        }

        // color related functions
        namespace color {

            // colorizes a string
            std::string colorize(const std::vector<Value> &colors, const std::string &value, bool reset) {

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

            std::string B(const std::string &value) {
                return colorize({fg::BLACK}, value, true);
            }

            std::string r(const std::string &value) {
                return colorize({fg::RED}, value, true);
            }

            std::string g(const std::string &value) {
                return colorize({fg::GREEN}, value, true);
            }

            std::string y(const std::string &value) {
                return colorize({fg::YELLOW}, value, true);
            }

            std::string b(const std::string &value) {
                return colorize({fg::BLUE}, value, true);
            }

            std::string m(const std::string &value) {
                return colorize({fg::MAGENTA}, value, true);
            }

            std::string c(const std::string &value) {
                return colorize({fg::CYAN}, value, true);
            }

            std::string w(const std::string &value) {
                return colorize({fg::WHITE}, value, true);
            }
        }

        // other terminal codes
        namespace vt100 {

            // output related functions
            namespace output {

                // called when a new line is added
                Callback on_newline;

                const Callback& Callback::operator()() const {
                    for (const auto &entry : values_) {
                        entry.second();
                    }
                    return *this;
                }

                Callback& Callback::add(std::size_t key, const Type &value) {
                    values_[key] = value;
                    return *this;
                }
                Callback& Callback::remove(std::size_t key) {
                    values_.erase(key);
                    return *this;
                }

                // utility for variadic print
                std::ostream &print(std::ostream &os) {
                    return os;
                }

                // output functions have their own mutex
                Mutex &get_mutex() {
                    static Mutex m;
                    return m;
                }
            }
            namespace cursor {

                Mutex &get_mutex() {
                    static Mutex m;
                    return m;
                }

                void set(int rows, int cols) {
                    if (internal::valid_term && rows > 0 && cols > 0) {
                        print("\033[", rows, ";", cols, "H") << std::flush;
                    }
                }

                void get(int *row, int *col) {
                    if (!internal::valid_term) {
                        return;
                    }

                    // disable echo and setup tty for reading
                    internal::TTYRead ttyRead;

                    std::string line;

                    // request the cursor position
                    print(cursor::REPORT) << std::flush;

                    // read the response
                    if (internal::read_report(STDIN_FILENO, line)) {
                        log::perror(errno);
                    }

                    // parse the response
                    if (sscanf(line.c_str(), "\033[%d;%dR", row, col) != 2) {
                        log::error("unable to parse cursor position [", line, "]");
                    };

                };

                Savepoint::Savepoint(Mutex &mutex) noexcept : guard_(mutex) {
                    // save the cursor position
                    print(SAVE) << std::flush;
                }

                Savepoint::~Savepoint() {
                    // restore the cursor position
                    print(RESTORE) << std::flush;
                }
            }

            void init() {
                // get the type of terminal
                auto term = std::getenv("TERM");

                // test validity
                internal::valid_term = term != nullptr && internal::init(term);

                if (internal::valid_term) {

                    cursor::Savepoint savepoint(cursor::get_mutex());

                    // find the screen height by setting the cursor to a extreme value
                    cursor::set(99999, 99999);

                    // get the actual position
                    cursor::get(&internal::rows, &internal::cols);
                }
            }

            Progress::Progress() noexcept : alive_(false), row_(0), callback_(),
                                            bg_() {

                if (internal::valid_term) {
                    init();

                    // start updating
                    bg_ = std::thread(std::bind(&Progress::run, this));
                }
            }

            Progress::~Progress() {
                alive_ = false;

                if (bg_.joinable()) {
                    bg_.join();
                }

                reset();
            }

            std::size_t Progress::key() const {
                return std::hash<const Progress*>()(this);
            }

            void Progress::reset() {
                if (!internal::valid_term) {
                    return;
                }

                // remove the new line callback
                output::on_newline.remove(key());

                // use a save point to reset the current cursor position
                cursor::Savepoint savepoint(cursor::get_mutex());

                // restore the cursor to this progress indicator
                cursor::set(row_, 2);

                // and erase the line
                print(cursor::ERASE_BACK) << std::flush;

            }

            void Progress::init() {

                int col = 0;

                std::lock_guard<Mutex> lock(cursor::get_mutex());

                // get the current row
                cursor::get(&row_, &col);

                callback_ = [&]() {
                    // if we're going to scroll at the last row...
                    if (row_ == internal::rows) {
                        // move the reset point back
                        --row_;
                    }
                };

                // add the callback for new lines
                output::on_newline.add(key(), callback_);
            }

            void Progress::run() {
                using namespace std::chrono_literals;

                alive_ = true;

                while (alive_) {
                    update();
                    std::this_thread::sleep_for(100ms);
                }
            }

            void Progress::update() {
                static unsigned frame = 0;
                constexpr static const char *FRAMES[] = {"◜", "◠", "◝", "◞", "◡", "◟"};
                constexpr static size_t FRAME_SIZE = (sizeof(FRAMES) / sizeof(FRAMES[0]));

                ++frame;
                frame %= FRAME_SIZE;

                // use a save point to reset the current cursor position
                cursor::Savepoint savepoint(cursor::get_mutex());

                // set the cursor to the saved position
                cursor::set(row_, 1);

                // send the frame and move the cursor back one character to print again
                print(color::colorize({color::attr::BOLD, color::fg::GREEN}, FRAMES[frame], true))
                        << std::flush;
            }
        }
    }
}