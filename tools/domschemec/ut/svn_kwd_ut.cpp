#include "vfs.h"
#include "../svn_keywords.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(ParserSvnKeywordsTest) {
    Y_UNIT_TEST(ExtractSvnKeywordValue) {
        UNIT_ASSERT(IsSvnKeyword(TStringBuf("$Revision$")));
        UNIT_ASSERT_STRINGS_EQUAL(ExtractSvnKeywordValue(TStringBuf("$Revision$")), "0");
        UNIT_ASSERT_STRINGS_EQUAL(ExtractSvnKeywordValue(TStringBuf("\"$Revision$\"")), "\"\"");
        UNIT_ASSERT(IsSvnKeyword(TStringBuf("$Rev: 123$")));
        UNIT_ASSERT_STRINGS_EQUAL(ExtractSvnKeywordValue(TStringBuf("$Rev: 123$")), "123");
        UNIT_ASSERT(IsSvnKeyword(TStringBuf("\"$Rev: 123$\"")));
        UNIT_ASSERT_STRINGS_EQUAL(ExtractSvnKeywordValue(TStringBuf("\"$Rev: 123$\"")), "\"123\"");
        UNIT_ASSERT(IsSvnKeyword(TStringBuf("$Rev: 123 $")));
        UNIT_ASSERT_STRINGS_EQUAL(ExtractSvnKeywordValue(TStringBuf("$Rev: 123 $")), "123");
        UNIT_ASSERT(IsSvnKeyword(TStringBuf("\"$Rev: 123 $\"")));
        UNIT_ASSERT_STRINGS_EQUAL(ExtractSvnKeywordValue(TStringBuf("\"$Rev: 123 $\"")), "\"123\"");
        // NB: there should not be any spaces between $ and "
        UNIT_ASSERT(!IsSvnKeyword(TStringBuf("\"$Rev: 123$ \"")));
        UNIT_ASSERT(!IsSvnKeyword(TStringBuf("\" $Rev: 123$\"")));
        // extra $ in the middle of value included into result
        UNIT_ASSERT(IsSvnKeyword(TStringBuf("\"$Rev: $ 123$\"")));
        UNIT_ASSERT_STRINGS_EQUAL(ExtractSvnKeywordValue(TStringBuf("\"$Rev: $ 123$\"")), "\"$ 123\"");
    }
    Y_UNIT_TEST(Simple) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace A;
pragma process_svn_keywords;
struct A {
    f : ui32 (const = $Rev: 123$);
    g : string (default = "$Rev: 789$");
};
)"}});
        TString result;
        UNIT_ASSERT_NO_EXCEPTION(result = Process(vfs, "./main.sc"));
        UNIT_ASSERT_STRING_CONTAINS(result, "F() = 123;");
        UNIT_ASSERT_STRING_CONTAINS(result, "G() = \"789\";");
    }
    Y_UNIT_TEST(FailToParseSvnKwdWithoutPragma) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace A;
struct A {
    f : ui32 (const = $Rev: 123$);
    g : string (default = "$Rev: 789$");
};
)"},
});
        UNIT_ASSERT_EXCEPTION_SATISFIES(Process(vfs, "./main.sc"), NDomSchemeCompiler::TParseError,
          [](const NDomSchemeCompiler::TParseError& e){ return e.Message.Contains("Unexpected token 'Rev'"); });
    }
    Y_UNIT_TEST(SnvKwdParsingIsStrict) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace A;
pragma process_svn_keywords;
struct A {
    g : string (default = "$Rev: 789$ ");
};
)"}});
        TString result;
        UNIT_ASSERT_NO_EXCEPTION(result = Process(vfs, "./main.sc"));
        UNIT_ASSERT_STRING_CONTAINS(result, "G() = \"$Rev: 789$ \";");
    }
    Y_UNIT_TEST(PragmaOff) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace A;
pragma process_svn_keywords;
struct A {
    f : string (default = "$Rev: 789$");
};
pragma_off process_svn_keywords;
struct B {
    g : string (default = "$Rev: 789$");
};
)"}});
        TString result;
        UNIT_ASSERT_NO_EXCEPTION(result = Process(vfs, "./main.sc"));
        UNIT_ASSERT_STRING_CONTAINS(result, "F() = \"789\";");
        UNIT_ASSERT_STRING_CONTAINS(result, "G() = \"$Rev: 789$\";");
    }
    Y_UNIT_TEST(PragmaDoesNotDistributeOverInclude) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace A;
pragma process_svn_keywords;
include "b.sc";
struct A {
    f : string (default = "$Rev: 789$");
};
)"},

           {"./b.sc", R"(
struct B {
    g : string (default = "$Rev: 789$");
};
)"}});
        TString result;
        UNIT_ASSERT_NO_EXCEPTION(result = Process(vfs, "./main.sc"));
        UNIT_ASSERT_STRING_CONTAINS(result, "F() = \"789\";");
        UNIT_ASSERT_STRING_CONTAINS(result, "G() = \"$Rev: 789$\";");
    }
};
