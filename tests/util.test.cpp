#include <bandit/bandit.h>

#include "util.h"

using namespace micrantha;
using namespace bandit;

go_bandit([]() {

    describe("filesystem", []() {
        it("can create a temp directory", []() {
            auto path = prep::make_temp_dir();

            Assert::That(path.empty(), IsFalse());
        });

        it("can remove", []() {

        });
    });

});
