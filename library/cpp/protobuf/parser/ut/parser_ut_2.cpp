#include "parser_ut.h"

namespace NImpl {
    void FieldMergeTest2() {
    //
    // int32 & sint32 & sfixed32
    // int64 & sint64 & sfixed64
    //

#undef COMPARE_RESULTS
#define COMPARE_RESULTS(canon, real) UNIT_ASSERT_EQUAL(canon, real)

#define TEST_CONVERSION_INT_TYPES(from, result)                     \
    TEST_CONVERSION_FIELD(int32, from, static_cast<i32>(result))    \
    TEST_CONVERSION_FIELD(sint32, from, static_cast<i32>(result))   \
    TEST_CONVERSION_FIELD(sfixed32, from, static_cast<i32>(result)) \
    TEST_CONVERSION_FIELD(int64, from, static_cast<i64>(result))    \
    TEST_CONVERSION_FIELD(sint64, from, static_cast<i64>(result))   \
    TEST_CONVERSION_FIELD(sfixed64, from, static_cast<i64>(result))

#define TEST_BAD_CAST_INT_TYPES(old, from)                     \
    TEST_BAD_CAST_FIELD(int32, static_cast<i32>(old), from)    \
    TEST_BAD_CAST_FIELD(sint32, static_cast<i32>(old), from)   \
    TEST_BAD_CAST_FIELD(sfixed32, static_cast<i32>(old), from) \
    TEST_BAD_CAST_FIELD(int64, static_cast<i64>(old), from)    \
    TEST_BAD_CAST_FIELD(sint64, static_cast<i64>(old), from)   \
    TEST_BAD_CAST_FIELD(sfixed64, static_cast<i64>(old), from)

        TEST_CONVERSION_INT_TYPES(1.2, 1);
        TEST_CONVERSION_INT_TYPES(-1.2f, -1);
        TEST_CONVERSION_INT_TYPES(false, 0);
        TEST_CONVERSION_INT_TYPES(true, 1);
        TEST_CONVERSION_INT_TYPES(static_cast<ui64>(100), 100);
        TEST_CONVERSION_INT_TYPES(static_cast<ui32>(0), 0);
        TEST_CONVERSION_INT_TYPES(static_cast<i64>(-1010), -1010);
        TEST_CONVERSION_INT_TYPES(static_cast<i32>(-95555), -95555);
        TEST_CONVERSION_INT_TYPES(TStringBuf("-10"), -10);
        TEST_CONVERSION_INT_TYPES(TString("3353"), 3353);
        TEST_BAD_CAST_INT_TYPES(33, TString("true"));
        TEST_BAD_CAST_INT_TYPES(0, TStringBuf("___"))
        TEST_BAD_CAST_INT_TYPES(33, TString("beleberda"));
        TEST_BAD_CAST_INT_TYPES(0, TStringBuf("1one"));
        TEST_BAD_CAST_INT_TYPES(0, TStringBuf("3.14"));
        TEST_BAD_CAST_INT_TYPES(0, TStringBuf("-3.14"));
        TEST_BAD_CAST_FIELD(int32, 3, static_cast<i64>(Max<i32>()) + 10);
    }
}
