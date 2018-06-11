#include <bandit/bandit.h>
#include <fstream>
#include <common.h>
#include "util.h"

using namespace micrantha;
using namespace bandit;

go_bandit([]() {

    describe("filesystem", []() {
        using namespace prep::filesystem;

        it("can create a temp directory", []() {
            auto path = make_temp_dir();

            Assert::That(path.empty(), IsFalse());

            Assert::That(directory_exists(path), Equals(PREP_SUCCESS));
        });

        it("can remove", []() {
            auto path = make_temp_dir();

            Assert::That(directory_exists(path), Equals(PREP_SUCCESS));

            Assert::That(remove_directory(path), Equals(PREP_SUCCESS));

            Assert::That(directory_exists(path), !Equals(PREP_SUCCESS));
        });

        it("can build a system path", []() {
            auto path = build_path("abc", "def", "xyz");

            Assert::That(path, Equals("abc/def/xyz"));
        });

        it("can remove a directory with contents", []() {

            auto path = make_temp_dir();

            auto fName = build_path(path, "test.file");

            std::ofstream f(fName);

            f << "testing\n";

            f.close();

            Assert::That(file_exists(fName), Equals(PREP_SUCCESS));

            Assert::That(remove_directory(path), Equals(PREP_SUCCESS));

            Assert::That(directory_exists(path), !Equals(PREP_SUCCESS));
        });
    });

    describe("io", []() {

        it("can read a file descriptor", []() {

        });
    });

    describe("process", []() {

    });
});
