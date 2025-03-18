#include "parser_ut.h"

namespace NImpl {
    void FieldMergeTest3() {
    //
    // uint32 & fixed32
    // uint64 & fixed64
    //
#undef COMPARE_RESULTS
#define COMPARE_RESULTS(canon, real) UNIT_ASSERT_EQUAL(canon, real)

#define TEST_CONVERSION_UINT_TYPES(from, result)                    \
    TEST_CONVERSION_FIELD(uint32, from, static_cast<ui32>(result))  \
    TEST_CONVERSION_FIELD(fixed32, from, static_cast<ui32>(result)) \
    TEST_CONVERSION_FIELD(uint64, from, static_cast<ui64>(result))  \
    TEST_CONVERSION_FIELD(fixed64, from, static_cast<ui64>(result))

#define TEST_BAD_CAST_UINT_TYPES(old, from)                    \
    TEST_BAD_CAST_FIELD(uint32, static_cast<ui32>(old), from)  \
    TEST_BAD_CAST_FIELD(fixed32, static_cast<ui32>(old), from) \
    TEST_BAD_CAST_FIELD(uint64, static_cast<ui64>(old), from)  \
    TEST_BAD_CAST_FIELD(fixed64, static_cast<ui64>(old), from)

        TEST_CONVERSION_UINT_TYPES(1.2, 1);
        TEST_CONVERSION_UINT_TYPES(1.8f, 1);
        TEST_CONVERSION_UINT_TYPES(false, 0);
        TEST_CONVERSION_UINT_TYPES(true, 1);
        TEST_CONVERSION_UINT_TYPES(static_cast<ui64>(100), 100);
        TEST_CONVERSION_UINT_TYPES(static_cast<ui32>(0), 0);
        TEST_CONVERSION_UINT_TYPES(static_cast<i64>(1010), 1010);
        TEST_CONVERSION_UINT_TYPES(static_cast<i32>(95555), 95555);
        TEST_CONVERSION_UINT_TYPES(TStringBuf("10"), 10);
        TEST_CONVERSION_UINT_TYPES(TString("3353"), 3353);
        TEST_BAD_CAST_UINT_TYPES(33, TString("true"));
        TEST_BAD_CAST_UINT_TYPES(0, TStringBuf("___"))
        TEST_BAD_CAST_UINT_TYPES(33, TString("beleberda"));
        TEST_BAD_CAST_UINT_TYPES(0, TStringBuf("1one"));
        TEST_BAD_CAST_UINT_TYPES(0, TStringBuf("3.14"));
        TEST_BAD_CAST_UINT_TYPES(0, TStringBuf("-3.14"));
        TEST_BAD_CAST_UINT_TYPES(0, TStringBuf("-3"));
        TEST_BAD_CAST_FIELD(uint32, 3, static_cast<ui64>(Max<ui32>()) + 10);
        TEST_BAD_CAST_FIELD(uint64, 6, -5);
        TEST_BAD_CAST_FIELD(fixed64, 6, -5);
    }
}
