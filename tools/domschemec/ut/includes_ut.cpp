#include "vfs.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(ParserIncludeTest) {
    Y_UNIT_TEST(Simple) {
        TVFSFileResolver::TVFS vfs({
            {"./main1.sc", R"(
namespace N1::N2;

struct TA {
    sa(required): string;
};

include "inc1.sc";

struct TC {
    sa(required): string;
};
)"},
            {"./main2.sc", R"(
namespace N1::N2;

struct TA {
    sa(required): string;
};

struct TC {
    sa(required): string;
};
)"},
            {"./inc1.sc", R"(
struct TB {
    ta(required): TA;
};
)"}});
        TString result;

        UNIT_ASSERT_NO_EXCEPTION(result = Process(vfs, "./main1.sc"));
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TAConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TBConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TCConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TA");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TB");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TC");

        UNIT_ASSERT_NO_EXCEPTION(result = Process(vfs, "./main2.sc"));
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TAConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TCConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TA");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TC");
        UNIT_ASSERT_TEST_FAILS(({UNIT_ASSERT_STRING_CONTAINS(result, "struct TBConst");}));
        UNIT_ASSERT_TEST_FAILS(({UNIT_ASSERT_STRING_CONTAINS(result, "struct TB");}));

    }
    Y_UNIT_TEST(Nested) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace N1::N2;
struct TA {
    sa(required): string;
};
include "inc1.sc";
struct TC {
    sa(required): string;
};
)"},
            {"./inc1.sc", R"(
struct TB {
    ta(required): TA;
};
include "inc2.sc";
struct TE {
    td(required): TD;
};)"},
            {"./inc2.sc", R"(
struct TD {
    tb(required): TB;
    ta(required): TA;
};)"}
});
        TString result;

        UNIT_ASSERT_NO_EXCEPTION(result = Process(vfs, "./main.sc"));
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TAConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TCConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TA");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TC");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TBConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TEConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TB");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TE");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TDConst");
        UNIT_ASSERT_STRING_CONTAINS(result, "struct TD");
    }
    Y_UNIT_TEST(SyntaxError) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace N1::N2;
include "inc1.sc;
)"},
            {"./inc1.sc", R"(
struct TB {
    ta(required): TA;
};)"}
});
        UNIT_ASSERT_EXCEPTION_SATISFIES(Process(vfs, "./main.sc"), NDomSchemeCompiler::TParseError,
          [](const NDomSchemeCompiler::TParseError& e){ return e.Message.Contains("Expected string"); });
    }
    Y_UNIT_TEST(FileNotFoundError) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace N1::N2;
include "inc1.sc";
)"},
});
        UNIT_ASSERT_EXCEPTION_SATISFIES(Process(vfs, "./main.sc"), NDomSchemeCompiler::TParseError,
          [](const NDomSchemeCompiler::TParseError& e){ return e.Message.Contains("Cannot include file"); });
    }
    Y_UNIT_TEST(IncludesAllowedOnlyAtTypesLevel) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace N1::N2;
struct TA {
include "inc1.sc";
   qwe : string;
};
)"},
            {"./inc1.sc", R"(
namespace N3;
struct TB {
    abc: string;
};)"}
});
        UNIT_ASSERT_EXCEPTION_SATISFIES(Process(vfs, "./main.sc"), NDomSchemeCompiler::TParseError,
          [](const NDomSchemeCompiler::TParseError& e){ return e.Message.Contains("Unexpected token"); });
    }
};
