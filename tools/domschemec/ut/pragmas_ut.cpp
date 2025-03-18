#include "vfs.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(Pragmas) {
    Y_UNIT_TEST(ExceptionOnUnknownPragma) {
        TVFSFileResolver::TVFS vfs({
            {"./main.sc", R"(
namespace A;
pragma some_unknown_pragma;
struct A {
  a : i32;
};
)"},
});
        UNIT_ASSERT_EXCEPTION_SATISFIES(Process(vfs, "./main.sc"), NDomSchemeCompiler::TParseError,
          [](const NDomSchemeCompiler::TParseError& e){ return e.Message.Contains("Unknown pragma: 'some_unknown_pragma'"); });
    }
};
