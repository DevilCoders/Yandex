#include "parser_ut.h"

namespace NImpl {
    void FieldMergeTest5() {
    //
    // string & bytes
    //

#undef COMPARE_RESULTS
#define COMPARE_RESULTS(canon, real) UNIT_ASSERT_STRINGS_EQUAL(canon, real)

#define TEST_CONVERSION_STRING_TYPES(from, result) \
    TEST_CONVERSION_FIELD(string, from, result)    \
    TEST_CONVERSION_FIELD(bytes, from, result)

#define TEST_BAD_CAST_STRING_TYPES(old, from) \
    TEST_BAD_CAST_FIELD(string, old, from)    \
    TEST_BAD_CAST_FIELD(bytes, old, from)

        TEST_CONVERSION_STRING_TYPES(1.2, "1.2");
        TEST_CONVERSION_STRING_TYPES(1.8f, "1.8");
        TEST_CONVERSION_STRING_TYPES(false, "0");
        TEST_CONVERSION_STRING_TYPES(true, "1");
        TEST_CONVERSION_STRING_TYPES(static_cast<ui64>(100), "100");
        TEST_CONVERSION_STRING_TYPES(static_cast<ui32>(0), "0");
        TEST_CONVERSION_STRING_TYPES(static_cast<i64>(1010), "1010");
        TEST_CONVERSION_STRING_TYPES(static_cast<i32>(-95555), "-95555");
        TEST_CONVERSION_STRING_TYPES(TStringBuf("10"), "10");
        TEST_CONVERSION_STRING_TYPES(TString("beleberda"), "beleberda");
    }
}
