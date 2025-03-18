#include "parser_ut.h"

namespace NImpl {
    void FieldMergeTest1() {
    //
    // double & float
    //

#define COMPARE_RESULTS(canon, real) UNIT_ASSERT_DOUBLES_EQUAL(canon, real, 0.0001)

#define TEST_CONVERSION_FLOAT_TYPES(from, result) \
    TEST_CONVERSION_FIELD(double, from, result)   \
    TEST_CONVERSION_FIELD(float, from, result)
#define TEST_BAD_CAST_FLOAT_TYPES(old, from) \
    TEST_BAD_CAST_FIELD(double, old, from)   \
    TEST_BAD_CAST_FIELD(float, old, from)

        TEST_CONVERSION_FLOAT_TYPES(1.1, 1.1);
        TEST_CONVERSION_FLOAT_TYPES(1.2f, 1.2);
        TEST_CONVERSION_FLOAT_TYPES(false, 0.0);
        TEST_CONVERSION_FLOAT_TYPES(true, 1.0);
        TEST_CONVERSION_FLOAT_TYPES(static_cast<ui64>(100), 100.0);
        TEST_CONVERSION_FLOAT_TYPES(static_cast<ui32>(0), 0.0);
        TEST_CONVERSION_FLOAT_TYPES(static_cast<i64>(-1010), -1010.0);
        TEST_CONVERSION_FLOAT_TYPES(static_cast<i32>(-95555), -95555.0);
        TEST_CONVERSION_FLOAT_TYPES(TStringBuf("-10.7"), -10.7);
        TEST_CONVERSION_FLOAT_TYPES(TString("3.14"), 3.14);
        TEST_BAD_CAST_FLOAT_TYPES(33.0, TString("true"));
        TEST_BAD_CAST_FLOAT_TYPES(0.0, TStringBuf("___"))
        TEST_BAD_CAST_FLOAT_TYPES(33.0, TString("beleberda"));
        TEST_BAD_CAST_FLOAT_TYPES(0.0, TStringBuf("1one"));
    }
}
