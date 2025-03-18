#include "parser_ut.h"

namespace NImpl {
    void FieldMergeTest4() {
#undef COMPARE_RESULTS
#define COMPARE_RESULTS(canon, real) UNIT_ASSERT_EQUAL(canon, real)

        //
        // bool
        //

        TEST_CONVERSION_FIELD(bool, 1.2, true);
        TEST_CONVERSION_FIELD(bool, 0.0f, false);
        TEST_CONVERSION_FIELD(bool, false, false);
        TEST_CONVERSION_FIELD(bool, true, true);
        TEST_CONVERSION_FIELD(bool, static_cast<ui64>(100), true);
        TEST_CONVERSION_FIELD(bool, static_cast<ui32>(0), false);
        TEST_CONVERSION_FIELD(bool, static_cast<i64>(-1010), true);
        TEST_CONVERSION_FIELD(bool, static_cast<i32>(0), false);
        TEST_CONVERSION_FIELD(bool, "true", true);
        TEST_CONVERSION_FIELD(bool, TStringBuf("0"), false);
        TEST_CONVERSION_FIELD(bool, TString("1"), true);
        TEST_CONVERSION_FIELD(bool, TString("FALSE"), false);
        TEST_BAD_CAST_FIELD(bool, true, TString("beleberda"));
        TEST_BAD_CAST_FIELD(bool, false, TStringBuf("1one"));
        TEST_BAD_CAST_FIELD(bool, false, TStringBuf("3.14"));
        TEST_BAD_CAST_FIELD(bool, true, TStringBuf("-3.14"));
    }
}
