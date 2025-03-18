#include <library/cpp/clickhouse/client/types/type_parser.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NClickHouse;

Y_UNIT_TEST_SUITE(TTypeParserTest){
    Y_UNIT_TEST(ParseTerminals){
        TTypeAst ast;
TTypeParser("UInt8").Parse(&ast);

UNIT_ASSERT_EQUAL(ast.Meta, TTypeAst::Terminal);
UNIT_ASSERT_EQUAL(ast.Name, "UInt8");
}

Y_UNIT_TEST(ParseFixedString) {
    TTypeAst ast;
    TTypeParser("FixedString(24)").Parse(&ast);

    UNIT_ASSERT_EQUAL(ast.Meta, TTypeAst::Terminal);
    UNIT_ASSERT_EQUAL(ast.Name, "FixedString");
    UNIT_ASSERT_EQUAL(ast.Elements.front().Value, 24);
}

Y_UNIT_TEST(ParseArray) {
    TTypeAst ast;
    TTypeParser("Array(Int32)").Parse(&ast);

    UNIT_ASSERT_EQUAL(ast.Meta, TTypeAst::Array);
    UNIT_ASSERT_EQUAL(ast.Name, "Array");
    UNIT_ASSERT_EQUAL(ast.Elements.front().Meta, TTypeAst::Terminal);
    UNIT_ASSERT_EQUAL(ast.Elements.front().Name, "Int32");
}

Y_UNIT_TEST(ParseNullable) {
    TTypeAst ast;
    TTypeParser("Nullable(Date)").Parse(&ast);

    UNIT_ASSERT_EQUAL(ast.Meta, TTypeAst::Nullable);
    UNIT_ASSERT_EQUAL(ast.Name, "Nullable");
    UNIT_ASSERT_EQUAL(ast.Elements.front().Meta, TTypeAst::Terminal);
    UNIT_ASSERT_EQUAL(ast.Elements.front().Name, "Date");
}

Y_UNIT_TEST(ParseEnum) {
    TTypeAst ast;
    TTypeParser(
        "Enum8('COLOR_red_10_T' = -12, 'COLOR_green_20_T'=-25, 'COLOR_blue_30_T'= 53, 'COLOR_black_30_T' = 107")
        .Parse(&ast);
    UNIT_ASSERT_EQUAL(ast.Meta, TTypeAst::Enum);
    UNIT_ASSERT_EQUAL(ast.Name, "Enum8");
    UNIT_ASSERT_EQUAL(ast.Elements.size(), 4);

    TVector<TString> names = {"COLOR_red_10_T", "COLOR_green_20_T", "COLOR_blue_30_T", "COLOR_black_30_T"};
    TVector<i64> values = {-12, -25, 53, 107};

    auto element = ast.Elements.begin();
    for (size_t i = 0; i < 4; ++i) {
        UNIT_ASSERT_EQUAL(element->Name, names[i]);
        UNIT_ASSERT_EQUAL(element->Value, values[i]);
        ++element;
    }
}
}
;
