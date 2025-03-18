#include <tools/domschemec/lexer_context.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NDomSchemeCompiler;

Y_UNIT_TEST_SUITE(DefaultFileRosolver) {
    Y_UNIT_TEST(Simple) {
        TDefaultFileResolver resolver("test.sc");
        UNIT_ASSERT_STRINGS_EQUAL(resolver.FileName(), "test.sc");
    }
    Y_UNIT_TEST(ExceptionFileNotFound) {
        TDefaultFileResolver resolver("test.sc");
        UNIT_ASSERT_EXCEPTION_SATISFIES(resolver.Resolve("abracadabra.sc"), yexception,
          [](const yexception& e){ return ToString(e.what()).Contains("Cannot find file"); });
    }
};
