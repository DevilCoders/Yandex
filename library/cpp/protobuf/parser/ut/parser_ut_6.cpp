#include "parser_ut.h"

namespace NImpl {
    void FieldMergeTest6() {
    //
    // enum
    //

#undef COMPARE_RESULTS
#define COMPARE_RESULTS(canon, real) UNIT_ASSERT_EQUAL(canon, real)

        TEST_CONVERSION_FIELD(enum, static_cast<ui64>(10), NParserUt::test2);
        TEST_CONVERSION_FIELD(enum, static_cast<ui32>(1), NParserUt::test1);
        TEST_CONVERSION_FIELD(enum, static_cast<i64>(10), NParserUt::test2);
        TEST_CONVERSION_FIELD(enum, static_cast<i32>(1), NParserUt::test1);
        TEST_CONVERSION_FIELD(enum, static_cast<i32>(-5), NParserUt::test3);
        TEST_CONVERSION_FIELD(enum, TStringBuf("test1"), NParserUt::test1);
        TEST_CONVERSION_FIELD(enum, TString("test2"), NParserUt::test2);
        TEST_CONVERSION_FIELD(enum, TString("10"), NParserUt::test2);
        TEST_CONVERSION_FIELD(enum, TString("-5"), NParserUt::test3);
        TEST_BAD_CAST_FIELD(enum, NParserUt::test1, 1.0);
        TEST_BAD_CAST_FIELD(enum, NParserUt::test2, 10.0f);
        TEST_BAD_CAST_FIELD(enum, NParserUt::test2, true);
        TEST_BAD_CAST_FIELD(enum, NParserUt::test2, false);
        TEST_BAD_CAST_FIELD(enum, NParserUt::test1, TString("beleberda"));
        TEST_BAD_CAST_FIELD(enum, NParserUt::test3, TStringBuf("1one"));
        TEST_BAD_CAST_FIELD(enum, NParserUt::test3, TStringBuf("3.14"));
        TEST_BAD_CAST_FIELD(enum, NParserUt::test1, TStringBuf());
    }
}
