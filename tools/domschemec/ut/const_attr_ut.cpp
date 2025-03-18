#include "vfs.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(ParserConstAttrTest) {
    Y_UNIT_TEST(Simple) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace A;
struct TA {
    f : i64 (const = 42);
    g : i64 (default = 43);
};
)"}});
        TString result;

        UNIT_ASSERT_NO_EXCEPTION(result = Process(vfs, "./main.sc"));
        UNIT_ASSERT_STRING_CONTAINS(result, "G() const {");
        UNIT_ASSERT_STRING_CONTAINS(result, "G() {");
        UNIT_ASSERT_STRING_CONTAINS(result, "F() const {");
        UNIT_ASSERT_STRING_CONTAINS(result, "F() {");
        UNIT_ASSERT_STRING_CONTAINS(result, "!HasG()");
        UNIT_ASSERT(!result.Contains("!HasF()"));
//        UNIT_ASSERT_STRING_NOT_CONTAINS(result, "F() {");
    }
};
