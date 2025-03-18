#include <library/cpp/html/lexer/face.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NHtmlLexer;

class TLexerTest: public TTestBase {
    UNIT_TEST_SUITE(TLexerTest);
    UNIT_TEST(TestNull);
    UNIT_TEST_SUITE_END();

public:
    /// Test of text tokenization with zeroes.
    void TestNull() {
        TString data("<html>Some \0text</html>", 23);
        UNIT_ASSERT_VALUES_EQUAL(DoLexing(data.data(), data.size()), 3);
    }

private:
    size_t DoLexing(const char* data, size_t len) const {
        size_t count = 0;

        LexHtmlData(data, len, [&count](const TToken*, size_t tokenCount, const NHtml::TAttribute*) -> bool {
            count += tokenCount;
            return true;
        });

        return count;
    }
};

UNIT_TEST_SUITE_REGISTRATION(TLexerTest);
