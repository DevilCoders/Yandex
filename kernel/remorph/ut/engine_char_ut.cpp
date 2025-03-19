#include <kernel/remorph/engine/char/char.h>

#include <kernel/remorph/input/wtroka_input_symbol.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>
#include <util/charset/wide.h>
#include <util/string/split.h>
#include <util/string/vector.h>

using namespace NSymbol;
using namespace NReMorph;

namespace {

const static TString SPACE = " ";

inline TWtrokaInputSymbolPtr ToSymbol(size_t pos, const TString& text) {
    TWtrokaInputSymbolPtr res(new TWtrokaInputSymbol(pos, UTF8ToWide(text)));
    res->GetProperties().Set(PROP_SPACE_BEFORE);
    return res;
}

TWtrokaInputSymbols ToSymbols(const TString& text) {
    TWtrokaInputSymbols symbols;
    TVector<TString> v;
    StringSplitter(text.c_str(), text.c_str() + text.size()).SplitByString(SPACE.data()).AddTo(&v);
    for (size_t i = 0; i < v.size(); ++i) {
        symbols.push_back(ToSymbol(i, v[i]));
    }
    return symbols;
}

} // unnamed namespace

#define CHAR_TEST(name, rules) \
    void Check ## name(const TCharEngine& engine); \
    Y_UNIT_TEST(Test ## name) { \
        TCharEnginePtr engine = TCharEngine::Parse(rules); \
        Check ## name(*engine); \
    } \
    void Check ## name(const TCharEngine& engine)

#define CHECK_SYNTAX_ERROR(rules) UNIT_ASSERT_EXCEPTION(TCharEngine::Parse(rules), yexception)
#define CHECK_SYNTAX_VALID(rules) try { \
        (void)TCharEngine::Parse(rules); \
    } catch (...) { \
        UNIT_FAIL("Parsing of " #rules " throws an exception: " << CurrentExceptionMessage()); \
    }

#define CHECK_RESULT(res) UNIT_ASSERT(res)
#define CHECK_NO_RESULT(res) UNIT_ASSERT(!(res))

Y_UNIT_TEST_SUITE(EngineChar) {
    Y_UNIT_TEST(ParseValidSyntax) {
        CHECK_SYNTAX_VALID("rule test = /abc/;");
        CHECK_SYNTAX_VALID("rule test = /abc/i;"); // flags
        CHECK_SYNTAX_VALID("rule test(2.3) = /abc/;"); // priority
        CHECK_SYNTAX_VALID("rule test(0.3) = /abc/;"); // priority
        CHECK_SYNTAX_VALID("rule test(2) = /abc/;"); // priority
        CHECK_SYNTAX_VALID("rule test(-2) = /abc/;"); // priority
        CHECK_SYNTAX_VALID("rule test = /abc|cbe/;"); //regex
        CHECK_SYNTAX_VALID("rule test = /a+b*c?/;"); //regex
        CHECK_SYNTAX_VALID("rule test = /^(abc).$/;"); //regex
        CHECK_SYNTAX_VALID("rule test = /[:Ll:]/;"); //unicode set
        CHECK_SYNTAX_VALID("rule test = /[[^:Ll:]a-z]/;"); //unicode set
        CHECK_SYNTAX_VALID("rule test = /(<a1>abc)/;"); //named submatch
        CHECK_SYNTAX_VALID("rule test = /a{2}/;"); //repeat
        CHECK_SYNTAX_VALID("rule test = /a{2,2}/;"); //repeat extended
        CHECK_SYNTAX_VALID("rule test = /a{2,3}/;"); //repeat range
        CHECK_SYNTAX_VALID("rule test = /a{0,3}/;"); //repeat range from zero
        CHECK_SYNTAX_VALID("rule test = /a{2,}/;"); //repeat to inf
        CHECK_SYNTAX_VALID("rule test = /a{1}/;"); //repeat pseudo
        CHECK_SYNTAX_VALID("rule test = /a{0,1}/;"); //repeat as a?
        CHECK_SYNTAX_VALID("rule test = /a{0,}/;"); //repeat as a*
        CHECK_SYNTAX_VALID("rule test = /a{1,}/;"); //repeat as a+
    }

    Y_UNIT_TEST(ParseValidSyntaxDef) {
        CHECK_SYNTAX_VALID("def test = /abc/;");
        CHECK_SYNTAX_VALID("def test = /abc/i;"); // flags
        CHECK_SYNTAX_VALID("def test = /abc|cbe/;"); //regex
        CHECK_SYNTAX_VALID("def test = /a+b*c?/;"); //regex
        CHECK_SYNTAX_VALID("def test = /^(abc).$/;"); //regex
        CHECK_SYNTAX_VALID("def test = /[:Ll:]/;"); //unicode set
        CHECK_SYNTAX_VALID("def test = /[[^:Ll:]a-z]/;"); //unicode set
        CHECK_SYNTAX_VALID("def test = /(<a1>abc)/;"); //named submatch
        CHECK_SYNTAX_VALID("def test = /a{2}/;"); //repeat
        CHECK_SYNTAX_VALID("def test = /a{2,2}/;"); //repeat extended
        CHECK_SYNTAX_VALID("def test = /a{2,3}/;"); //repeat range
        CHECK_SYNTAX_VALID("def test = /a{0,3}/;"); //repeat range from zero
        CHECK_SYNTAX_VALID("def test = /a{2,}/;"); //repeat to inf
        CHECK_SYNTAX_VALID("def test = /a{1}/;"); //repeat pseudo
        CHECK_SYNTAX_VALID("def test = /a{0,1}/;"); //repeat as a?
        CHECK_SYNTAX_VALID("def test = /a{0,}/;"); //repeat as a*
        CHECK_SYNTAX_VALID("def test = /a{1,}/;"); //repeat as a+
    }

    Y_UNIT_TEST(ParseValidSyntaxEscape) {
        CHECK_SYNTAX_VALID("rule test = /\\\\\\\\/;"); //escape '\'
        CHECK_SYNTAX_VALID("rule test = /\\//;"); //escape '/'
        CHECK_SYNTAX_VALID("rule test = /\\^\\$\\*\\./;"); //escape
        CHECK_SYNTAX_VALID("rule test = /\\[\\]/;"); //escape
    }

    Y_UNIT_TEST(ParseValidSyntaxReference) {
        CHECK_SYNTAX_VALID(
            "def a = /abc/;\n"
            "rule test = /@{a}/;\n"
        );
        CHECK_SYNTAX_VALID(
            "def a = /abc/;\n"
            "def b = /abc/;\n"
            "rule test = /@{a}@{b}/;\n"
        );
    }

    Y_UNIT_TEST(ParseSyntaxError) {
        CHECK_SYNTAX_ERROR("rule = /abc/;");
        CHECK_SYNTAX_ERROR("/abc/;");
        CHECK_SYNTAX_ERROR("rule test = /abc;");
        CHECK_SYNTAX_ERROR("rule test = /abc/");
        CHECK_SYNTAX_ERROR("rule test(2,3) = /abc/;"); // priority
        CHECK_SYNTAX_ERROR("rule test(abc) = /abc/;"); // priority
        CHECK_SYNTAX_ERROR("rule test = /(abc/;"); //regex
        CHECK_SYNTAX_ERROR("rule test = /?abc/;"); //regex
        CHECK_SYNTAX_ERROR("rule test = /*abc/;"); //regex
        CHECK_SYNTAX_ERROR("rule test = /+abc/;"); //regex
        CHECK_SYNTAX_ERROR("rule test = /abc++/;"); //regex
        CHECK_SYNTAX_ERROR("rule test = /[:Ll:/;"); //unicode set
        CHECK_SYNTAX_ERROR("rule test = /^+/;"); //regex
        CHECK_SYNTAX_ERROR("rule test = /(+)/;"); //regex
        CHECK_SYNTAX_ERROR("rule test = /\\\\/;"); //unfinised quoted pair
        CHECK_SYNTAX_ERROR("rule test = /{2,3}/;"); //wrong repeater placement
        CHECK_SYNTAX_ERROR("rule test = /a{}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("rule test = /a{-1}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("rule test = /a{-1,3}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("rule test = /a{2,-1}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("rule test = /a{,}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("rule test = /a{,3}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("rule test = /a{0,0}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("rule test = /a{3,2}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("rule test = /a{1,0}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("rule test = //;");
    }

    Y_UNIT_TEST(ParseSyntaxErrorDef) {
        CHECK_SYNTAX_ERROR("def = /abc/;");
        CHECK_SYNTAX_ERROR("def test = /abc;");
        CHECK_SYNTAX_ERROR("def test = /abc/");
        CHECK_SYNTAX_ERROR("def test(2,3) = /abc/;"); // priority
        CHECK_SYNTAX_ERROR("def test(abc) = /abc/;"); // priority
        CHECK_SYNTAX_ERROR("def test = /(abc/;"); //regex
        CHECK_SYNTAX_ERROR("def test = /?abc/;"); //regex
        CHECK_SYNTAX_ERROR("def test = /*abc/;"); //regex
        CHECK_SYNTAX_ERROR("def test = /+abc/;"); //regex
        CHECK_SYNTAX_ERROR("def test = /abc++/;"); //regex
        CHECK_SYNTAX_ERROR("def test = /[:Ll:/;"); //unicode set
        CHECK_SYNTAX_ERROR("def test = /^+/;"); //regex
        CHECK_SYNTAX_ERROR("def test = /(+)/;"); //regex
        CHECK_SYNTAX_ERROR("def test = /\\\\/;"); //unfinised quoted pair
        CHECK_SYNTAX_ERROR("def test = /{2,3}/;"); //wrong repeater placement
        CHECK_SYNTAX_ERROR("def test = /a{}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("def test = /a{-1}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("def test = /a{-1,3}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("def test = /a{2,-1}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("def test = /a{,}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("def test = /a{,3}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("def test = /a{0,0}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("def test = /a{3,2}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("def test = /a{1,0}/;"); //wrong repeater
        CHECK_SYNTAX_ERROR("def test(2.3) = /abc/;"); // priority
        CHECK_SYNTAX_ERROR("def test(0.3) = /abc/;"); // priority
        CHECK_SYNTAX_ERROR("def test(2) = /abc/;"); // priority
        CHECK_SYNTAX_ERROR("def test(-2) = /abc/;"); // priority
        CHECK_SYNTAX_ERROR("def test = //;");
    }

    Y_UNIT_TEST(ParseSyntaxErrorReference) {
        CHECK_SYNTAX_ERROR("rule test = /@/;");
        CHECK_SYNTAX_ERROR("rule test = /@a/;");
        CHECK_SYNTAX_ERROR("rule test = /@{}/;");
        CHECK_SYNTAX_ERROR("rule test = /@{/;");
        CHECK_SYNTAX_ERROR("rule test = /@{a/;");
        CHECK_SYNTAX_ERROR("rule test = /@{!!!}/;");
        CHECK_SYNTAX_ERROR("rule test = /@{a!!!}/;");
    }

    Y_UNIT_TEST(ParseSyntaxErrorReferenceResolving) {
        CHECK_SYNTAX_ERROR(
            "def a = /abc/;\n"
            "rule test = /@{b}/;\n"
        );
    }

    Y_UNIT_TEST(ParseLogicError) {
        CHECK_SYNTAX_ERROR("rule test = /abc/a;"); // unknown flag
        CHECK_SYNTAX_ERROR("rule test = /[:Uu:]/;"); //unknown unicode set
    }

    Y_UNIT_TEST(ParseLogicErrorDef) {
        CHECK_SYNTAX_ERROR("def test = /abc/a;"); // unknown flag
        CHECK_SYNTAX_ERROR("def test = /[:Uu:]/;"); //unknown unicode set
    }

    CHAR_TEST(ResultSingle, "rule test = /def/;") {
        TWtrokaInputSymbols symbols = ToSymbols("abc def");
        TCharResultPtr res = engine.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->GetMatchedCount(), 1);
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, 0)->GetText(), u"def");
        UNIT_ASSERT_EQUAL(res->GetWholeExprInOriginalSequence(symbols), NRemorph::TSubmatch(1, 2));
    }

    CHAR_TEST(ResultMultiple, "rule test = /abc def/;") {
        TWtrokaInputSymbols symbols = ToSymbols("abc def");
        TCharResultPtr res = engine.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->GetMatchedCount(), 2);
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, 0)->GetText(), u"abc");
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, 1)->GetText(), u"def");
        UNIT_ASSERT_EQUAL(res->GetWholeExprInOriginalSequence(symbols), NRemorph::TSubmatch(0, 2));
    }

    CHAR_TEST(ResultSpaces, "rule test = / bc /;") {
        TWtrokaInputSymbols symbols = ToSymbols("a bc d");
        TCharResultPtr res = engine.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->GetMatchedCount(), 1);
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, 0)->GetText(), u"bc");
        UNIT_ASSERT_EQUAL(res->GetWholeExprInOriginalSequence(symbols), NRemorph::TSubmatch(1, 2));
    }

    CHAR_TEST(ResultNamedMatch, "rule test = /(<a>abc) (<b>def ghj)/;") {
        TWtrokaInputSymbols symbols = ToSymbols("q abc def ghj ik");
        TCharResultPtr res = engine.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->GetMatchedCount(), 3);
        UNIT_ASSERT_EQUAL(res->GetWholeExprInOriginalSequence(symbols), NRemorph::TSubmatch(1, 4));
        UNIT_ASSERT_EQUAL(res->NamedSubmatches.size(), 2);

        UNIT_ASSERT_EQUAL(res->NamedSubmatches.count("a"), 1);
        UNIT_ASSERT_EQUAL(res->NamedSubmatches.find("a")->second, NRemorph::TSubmatch(0, 1));

        TWtrokaInputSymbols symbolRange;
        res->ExtractMatched(symbols, res->NamedSubmatches.find("a")->second, symbolRange);
        UNIT_ASSERT_EQUAL(symbolRange.size(), 1);
        UNIT_ASSERT_EQUAL(symbolRange.front()->GetText(), u"abc");

        UNIT_ASSERT_EQUAL(res->NamedSubmatches.count("b"), 1);
        UNIT_ASSERT_EQUAL(res->NamedSubmatches.find("b")->second, NRemorph::TSubmatch(1, 3));

        symbolRange.clear();
        res->ExtractMatched(symbols, res->NamedSubmatches.find("b")->second, symbolRange);
        UNIT_ASSERT_EQUAL(symbolRange.size(), 2);
        UNIT_ASSERT_EQUAL(symbolRange.front()->GetText(), u"def");
        UNIT_ASSERT_EQUAL(symbolRange.back()->GetText(), u"ghj");
    }

    CHAR_TEST(BoundarySingle, "rule test = /bc/;") {
        CHECK_RESULT(engine.Search(ToSymbols("bc")));
        CHECK_RESULT(engine.Search(ToSymbols("a bc")));
        CHECK_RESULT(engine.Search(ToSymbols("bc d")));
        CHECK_RESULT(engine.Search(ToSymbols("a bc d")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("abc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("bcd")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("abcd")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("a bcd")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("abc d")));
    }

    CHAR_TEST(BoundaryMulti, "rule test = /bc de/;") {
        CHECK_RESULT(engine.Search(ToSymbols("bc de")));
        CHECK_RESULT(engine.Search(ToSymbols("a bc de")));
        CHECK_RESULT(engine.Search(ToSymbols("bc de f")));
        CHECK_RESULT(engine.Search(ToSymbols("a bc de f")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("abc de")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("bc def")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("a bc def")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("abc de f")));
    }

    CHAR_TEST(BoundarySpace, "rule test = / bc /;") {
        CHECK_RESULT(engine.Search(ToSymbols("a bc d")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("bc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("a bc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("bc d")));
    }

    CHAR_TEST(AnchorStart, "rule test = /^bc/;") {
        CHECK_RESULT(engine.Search(ToSymbols("bc")));
        CHECK_RESULT(engine.Search(ToSymbols("bc d")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("a bc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("a bc d")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("abc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("bcd")));
    }

    CHAR_TEST(AnchorEnd, "rule test = /bc$/;") {
        CHECK_RESULT(engine.Search(ToSymbols("bc")));
        CHECK_RESULT(engine.Search(ToSymbols("a bc")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("bc d")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("a bc d")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("abc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("bcd")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("a bcd")));
    }

    CHAR_TEST(AnyMatch, "rule test = /a.c/;") {
        CHECK_RESULT(engine.Search(ToSymbols("abc")));
        CHECK_RESULT(engine.Search(ToSymbols("adc")));
        CHECK_RESULT(engine.Search(ToSymbols("a c")));
        CHECK_RESULT(engine.Search(ToSymbols("a.c")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("abb")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("b.c")));
    }

    CHAR_TEST(DotMatch, "rule test = /a\\\\.c/;") {
        CHECK_RESULT(engine.Search(ToSymbols("a.c")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("abb")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("b.c")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("adc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("a c")));
    }

    CHAR_TEST(EscapedSets1, "rule test = /\\w+\\s+\\d+/;") {
        CHECK_RESULT(engine.Search(ToSymbols("abc 123")));
        CHECK_RESULT(engine.Search(ToSymbols("a 1")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("abc123")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("123")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("abc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("abc abc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("123 456")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("a c")));
    }

    CHAR_TEST(EscapedChars1, "rule test = /\\\\ \\^\\$\\*\\.\\//;") {
        CHECK_RESULT(engine.Search(ToSymbols("\\ ^$*./")));
    }

    CHAR_TEST(EscapedChars2, "rule test = /\\\\[ \\[/;") {
        CHECK_RESULT(engine.Search(ToSymbols("[ [")));
    }

    CHAR_TEST(UnicodeSet, "rule test = /[:Ll:]+[:Nd:]?/;") {
        CHECK_RESULT(engine.Search(ToSymbols("a1")));
        CHECK_RESULT(engine.Search(ToSymbols("abc1")));
        CHECK_RESULT(engine.Search(ToSymbols("abc")));
        CHECK_RESULT(engine.Search(ToSymbols("a")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("A1")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("Abc")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("abc#")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("12")));
    }

    CHAR_TEST(CaseFlag1, "rule test = /aBc123/i;") {
        CHECK_RESULT(engine.Search(ToSymbols("aBc123")));
        CHECK_RESULT(engine.Search(ToSymbols("abc123")));
        CHECK_RESULT(engine.Search(ToSymbols("Abc123")));
        CHECK_RESULT(engine.Search(ToSymbols("ABC123")));
        CHECK_RESULT(engine.Search(ToSymbols("AbC123")));
        CHECK_RESULT(engine.Search(ToSymbols("abC123")));

        CHECK_NO_RESULT(engine.Search(ToSymbols("bc123")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("123")));
        CHECK_NO_RESULT(engine.Search(ToSymbols("ab#123")));
    }

    CHAR_TEST(CaseFlag2, "rule test = /[:Lu:]+\\s[:Ll:]+\\s[:Nd:]+/i;") {
        CHECK_RESULT(engine.Search(ToSymbols("aa bb 12")));
        CHECK_RESULT(engine.Search(ToSymbols("aA bB 14")));
        CHECK_RESULT(engine.Search(ToSymbols("AA bb 15")));
        CHECK_RESULT(engine.Search(ToSymbols("AA BB 10")));
        CHECK_RESULT(engine.Search(ToSymbols("aa BB 31")));
    }

    CHAR_TEST(Match, "rule test = /abc/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abc")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("abc def")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("123 abc")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("123 abc def")));
    }

    CHAR_TEST(MatchAnchor, "rule test = /^abc$/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abc")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("abc def")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("123 abc")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("123 abc def")));
    }

    CHAR_TEST(Priority, "rule test = /ab?c|de/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abc")));
        CHECK_RESULT(engine.Match(ToSymbols("ac")));
        CHECK_RESULT(engine.Match(ToSymbols("de")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("abe")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("ae")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("c")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcde")));
    }

    CHAR_TEST(Bracket1, "rule test = /ab(c|d)e/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abce")));
        CHECK_RESULT(engine.Match(ToSymbols("abde")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("abc")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("de")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcde")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abe")));
    }

    CHAR_TEST(Bracket2, "rule test = /(ab)+c/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abc")));
        CHECK_RESULT(engine.Match(ToSymbols("ababc")));
        CHECK_RESULT(engine.Match(ToSymbols("abababc")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("ac")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abbc")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abbbc")));
    }

    CHAR_TEST(Repeat, "rule test = /a(bc){2}d/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abcbcd")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("ad")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcd")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcbcbcd")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("abbd")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcbd")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("accd")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("acbcd")));
    }

    CHAR_TEST(RepeatRange, "rule test = /a(bc){2,3}d/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abcbcd")));
        CHECK_RESULT(engine.Match(ToSymbols("abcbcbcd")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("ad")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcd")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcbcbcbcd")));
    }

    CHAR_TEST(RepeatRangeFromZero, "rule test = /a(bc){0,3}d/;") {
        CHECK_RESULT(engine.Match(ToSymbols("ad")));
        CHECK_RESULT(engine.Match(ToSymbols("abcd")));
        CHECK_RESULT(engine.Match(ToSymbols("abcbcd")));
        CHECK_RESULT(engine.Match(ToSymbols("abcbcbcd")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("abcbcbcbcd")));
    }

    CHAR_TEST(RepeatPseudoRange, "rule test = /a(bc){2,2}d/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abcbcd")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("ad")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcd")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcbcbcd")));
    }

    CHAR_TEST(RepeatToInf, "rule test = /a(bc){2,}d/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abcbcd")));
        CHECK_RESULT(engine.Match(ToSymbols("abcbcbcd")));
        CHECK_RESULT(engine.Match(ToSymbols("abcbcbcbcbcbcbcbcd")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("ad")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcd")));
    }

    CHAR_TEST(RepeatRseudo, "rule test = /a(bc){1}d/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abcd")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("ad")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abcbcd")));
    }

    CHAR_TEST(RepeatAsQuestion, "rule test = /a(bc){0,1}d/;") {
        CHECK_RESULT(engine.Match(ToSymbols("ad")));
        CHECK_RESULT(engine.Match(ToSymbols("abcd")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("abcbcd")));
    }

    CHAR_TEST(RepeatAsAsterisk, "rule test = /a(bc){0,}d/;") {
        CHECK_RESULT(engine.Match(ToSymbols("ad")));
        CHECK_RESULT(engine.Match(ToSymbols("abcd")));
        CHECK_RESULT(engine.Match(ToSymbols("abcbcd")));
        CHECK_RESULT(engine.Match(ToSymbols("abcbcbcbcbcd")));
    }

    CHAR_TEST(RepeatAsPlus, "rule test = /a(bc){1,}d/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abcd")));
        CHECK_RESULT(engine.Match(ToSymbols("abcbcd")));
        CHECK_RESULT(engine.Match(ToSymbols("abcbcbcbcbcd")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("ad")));
    }

    CHAR_TEST(Reference,
        "def a = /abc/;\n"
        "rule test = /@{a}/;\n"
    ) {
        CHECK_RESULT(engine.Match(ToSymbols("abc")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abd")));
    }

    CHAR_TEST(ReferenceCatenation,
        "def a = /abc/;\n"
        "def b = /def/;\n"
        "rule test = /@{a}@{b}/;\n"
    ) {
        CHECK_RESULT(engine.Match(ToSymbols("abcdef")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abc")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("def")));
    }

    CHAR_TEST(ReferenceIteration,
        "def a = /abc/;\n"
        "rule test = /@{a}+/;\n"
    ) {
        CHECK_RESULT(engine.Match(ToSymbols("abc")));
        CHECK_RESULT(engine.Match(ToSymbols("abcabc")));
        CHECK_RESULT(engine.Match(ToSymbols("abcabcabc")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("a")));
    }

    CHAR_TEST(RepeatRepeat, "rule test = /a(b{2}){1,2}/;") {
        CHECK_RESULT(engine.Match(ToSymbols("abb")));
        CHECK_RESULT(engine.Match(ToSymbols("abbbb")));

        CHECK_NO_RESULT(engine.Match(ToSymbols("ab")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abbb")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abbbbb")));
        CHECK_NO_RESULT(engine.Match(ToSymbols("abbbbbb")));
    }
}

Y_UNIT_TEST_SUITE(EngineCharBugfixes) {
    Y_UNIT_TEST(Factex3036) {
        CHECK_SYNTAX_ERROR("rule test = //;");
        CHECK_SYNTAX_ERROR("def test = //;");
    }

    CHAR_TEST(Factex3037,
        "rule test = /./;\n"
    ) {
        TCharResults results;
        engine.SearchAll(ToSymbols("a a"), results);
        UNIT_ASSERT_EQUAL(results.size(), 2u);
    }
}
