#include <kernel/remorph/tokenlogic/tlmatcher.h>
#include <kernel/remorph/tokenlogic/tlresult.h>
#include <kernel/remorph/tokenlogic/rule_parser.h>

#include <kernel/remorph/input/properties.h>
#include <kernel/remorph/input/input_symbol_util.h>
#include <kernel/remorph/input/wtroka_input_symbol.h>
#include <kernel/gazetteer/gazetteer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/bitmap.h>
#include <util/string/split.h>
#include <util/string/vector.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

using namespace NSymbol;
using namespace NTokenLogic;

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

TString GetNamedTokens(const TTokenLogicResultPtr& res, const TWtrokaInputSymbols& symbols, const TString& label) {
    typedef NSorted::TSimpleMap<TString, size_t>::const_iterator iterator;
    std::pair<iterator, iterator> range = res->NamedTokens.equal_range(label);
    TVector<TString> tokens;
    for (iterator i = range.first; i != range.second; ++i) {
        tokens.push_back(WideToUTF8(res->GetMatchedSymbol(symbols, i->second)->GetText()));
    }
    Sort(tokens.begin(), tokens.end());
    return JoinStrings(tokens, ",");
}

TString GetMatchedRules(const TTokenLogicResults& res) {
    TVector<TString> rules;
    for (TTokenLogicResults::const_iterator i = res.begin(); i != res.end(); ++i) {
        rules.push_back(i->Get()->RuleName);
    }
    ::Sort(rules.begin(), rules.end());
    return JoinStrings(rules, ",");
}

struct TArticleCollector {
    TVector<TString>& Titles;

    TArticleCollector(TVector<TString>& t)
        : Titles(t)
    {
    }
    bool operator() (const TArticlePtr& a) {
        Titles.push_back(WideToUTF8(a.GetTitle()));
        return false;
    }
};

TString GetMatchedGzt(const TTokenLogicResultPtr& res, const TWtrokaInputSymbols& symbols, size_t pos) {
    TDynBitMap ctx;
    TWtrokaInputSymbolPtr s = res->GetMatchedSymbol(symbols, pos, ctx);
    TVector<TString> titles;
    s->TraverseOnlyCtxArticles(ctx, TArticleCollector(titles));
    ::Sort(titles.begin(), titles.end());
    return JoinStrings(titles, ",");
}

TTokenLogicResultPtr Search(const TString& text, const NGzt::TGazetteer& gzt, const TMatcher& matcher) {
    TWtrokaInput input;
    matcher.CreateInput(input, ToSymbols(text), &gzt);
    return matcher.Search(input);
}

} // unnamed namespace

#define TL_TEST(name, rules) \
    void Check ## name(const TMatcher& matcher); \
    Y_UNIT_TEST(Test ## name) { \
        TMatcherPtr matcher = TMatcher::Parse(rules); \
        Check ## name(*matcher); \
    } \
    void Check ## name(const TMatcher& matcher)

#define TL_GZT_TEST(name, gztSrc, rules) \
    void Check ## name(const NGzt::TGazetteer& gzt, const TMatcher& matcher); \
    Y_UNIT_TEST(Test ## name) { \
        TGazetteerBuilder buider; \
        buider.BuildFromText(gztSrc); \
        THolder<TGazetteer> gzt(buider.MakeGazetteer()); \
        TMatcherPtr matcher = TMatcher::Parse(rules, gzt.Get()); \
        Check ## name(*gzt, *matcher); \
    } \
    void Check ## name(const NGzt::TGazetteer& gzt, const TMatcher& matcher)


#define CHECK_RESULT(res) UNIT_ASSERT(res)
#define CHECK_NO_RESULT(res) UNIT_ASSERT(!(res))

#define CHECK_TXT_MATCH(text) UNIT_ASSERT(matcher.Search(ToSymbols(text)))
#define CHECK_TXT_UNMATCH(text) UNIT_ASSERT(!(matcher.Search(ToSymbols(text))))

#define CHECK_GZT_MATCH(text) UNIT_ASSERT(::Search(text, gzt, matcher))
#define CHECK_GZT_UNMATCH(text) UNIT_ASSERT(!::Search(text, gzt, matcher))

#define CHECK_EMPTY_UNMATCH(symb) UNIT_ASSERT(!(matcher.Search(TWtrokaInputSymbols())))

#define CHECK_NAMED_TOKENS(res, symbols, label, expected) UNIT_ASSERT_EQUAL_C(expected, GetNamedTokens(res, symbols, label), ", actual=\"" << GetNamedTokens(res, symbols, label) << "\"")

#define CHECK_MATCHED_RULES(text, expected) \
    do { \
        TTokenLogicResults res; \
        matcher.SearchAll(ToSymbols(text), res); \
        UNIT_ASSERT_EQUAL_C(expected, GetMatchedRules(res), ", actual=\"" << GetMatchedRules(res) << "\""); \
    } while (false)

#define CHECK_GZT_MATCHED_RULES(text, expected) \
    do { \
        TTokenLogicResults res; \
        TWtrokaInput input; \
        matcher.CreateInput(input, ToSymbols(text), &gzt); \
        matcher.SearchAll(input, res); \
        UNIT_ASSERT_EQUAL_C(expected, GetMatchedRules(res), ", actual=\"" << GetMatchedRules(res) << "\""); \
    } while (false)

#define CHECK_SYNTAX_ERROR(rules) UNIT_ASSERT_EXCEPTION(TMatcher::Parse(rules), NTokenLogic::NPrivate::TSyntaxError)
#define CHECK_SYNTAX_VALID(rules, gzt) UNIT_ASSERT_NO_EXCEPTION(TMatcher::Parse(rules, gzt))


Y_UNIT_TEST_SUITE(TokenLogicBase) {
    TL_TEST(Single, "rule test = one;") {
        CHECK_TXT_MATCH("one two three");
        CHECK_TXT_MATCH("two one three");
        CHECK_TXT_MATCH("one two one");

        CHECK_TXT_UNMATCH("two three");
    }

    TL_TEST(Set, "rule test = [one two];") {
        CHECK_TXT_MATCH("one two three");
        CHECK_TXT_MATCH("one two one");

        CHECK_TXT_UNMATCH("three four");
    }

    TL_TEST(Logic, "rule test = [?reg=\"one\"];") {
        CHECK_TXT_MATCH("one two three");
        CHECK_TXT_MATCH("two one three");

        CHECK_TXT_UNMATCH("two three");
    }

    // Use "." because empty matches are forbidden
    TL_TEST(SingleNeg, "rule test = !one && .;") {
        CHECK_TXT_MATCH("two three");

        CHECK_TXT_UNMATCH("one two three");
        CHECK_TXT_UNMATCH("two one three");
    }

    TL_TEST(Eq, "rule test = one == 3;") {
        CHECK_TXT_MATCH("one one one");

        CHECK_TXT_UNMATCH("one one two");
        CHECK_TXT_UNMATCH("one one one two one");
    }

    TL_TEST(Neq, "rule test = one != 2;") {
        CHECK_TXT_MATCH("one one one");
        CHECK_TXT_MATCH("one");

        CHECK_TXT_UNMATCH("two three"); // Empty match is forbidden
        CHECK_TXT_UNMATCH("one two one");
    }

    TL_TEST(Gt, "rule test = one > 1;") {
        CHECK_TXT_MATCH("one one one");
        CHECK_TXT_MATCH("one one two");

        CHECK_TXT_UNMATCH("one");
        CHECK_TXT_UNMATCH("two three");
    }

    TL_TEST(Gte, "rule test = one >= 1;") {
        CHECK_TXT_MATCH("one one one");
        CHECK_TXT_MATCH("one one two");
        CHECK_TXT_MATCH("one");

        CHECK_TXT_UNMATCH("two three");
    }

    TL_TEST(Lt, "rule test = one < 3;") {
        CHECK_TXT_MATCH("one one two");
        CHECK_TXT_MATCH("one");

        CHECK_TXT_UNMATCH("two three"); // Empty match is forbidden
        CHECK_TXT_UNMATCH("one one one one");
    }

    TL_TEST(Lte, "rule test = one <= 1;") {
        CHECK_TXT_MATCH("one");

        CHECK_TXT_UNMATCH("two three"); // Empty match is forbidden
        CHECK_TXT_UNMATCH("one one one");
        CHECK_TXT_UNMATCH("one one two");
    }

    // Use "." because empty matches are forbidden
    TL_TEST(True, "rule test = true && .;") {
        CHECK_TXT_MATCH("one one one");
        CHECK_TXT_MATCH("one one two");
        CHECK_TXT_MATCH("one");
        CHECK_TXT_MATCH("two three");
        CHECK_EMPTY_UNMATCH();
    }

    TL_TEST(False, "rule test = false;") {
        CHECK_TXT_UNMATCH("one one one");
        CHECK_TXT_UNMATCH("one one two");
        CHECK_TXT_UNMATCH("one");
        CHECK_TXT_UNMATCH("two three");
        CHECK_EMPTY_UNMATCH();
    }

    TL_TEST(Any, "rule test = .;") {
        CHECK_TXT_MATCH("one one one");
        CHECK_TXT_MATCH("one one two");
        CHECK_TXT_MATCH("one");
        CHECK_TXT_MATCH("two three");

        CHECK_EMPTY_UNMATCH();
    }

    TL_TEST(Opt, "rule test = one?;") {
        CHECK_TXT_MATCH("one one one");
        CHECK_TXT_MATCH("one one two");
        CHECK_TXT_MATCH("one");

        CHECK_TXT_UNMATCH("two three"); // Empty match is forbidden
        CHECK_EMPTY_UNMATCH(); // Empty match is forbidden
    }
}

Y_UNIT_TEST_SUITE(TokenLogicExpression) {
    TL_TEST(And, "rule test = one > 1 && two > 1;") {
        CHECK_TXT_MATCH("one one two two");

        CHECK_TXT_UNMATCH("one two two");
        CHECK_TXT_UNMATCH("one two");
        CHECK_TXT_UNMATCH("one one two");
    }

    TL_TEST(Or, "rule test = one > 1 || two > 1;") {
        CHECK_TXT_MATCH("one two two");
        CHECK_TXT_MATCH("one one two");
        CHECK_TXT_MATCH("one one two two");

        CHECK_TXT_UNMATCH("one two");
    }

    TL_TEST(Xor, "rule test = one > 1 ^^ two > 1;") {
        CHECK_TXT_UNMATCH("one one two two");
        CHECK_TXT_UNMATCH("one two");

        CHECK_TXT_MATCH("one two two");
        CHECK_TXT_MATCH("one one two");
    }

    TL_TEST(Not, "rule test = !(one > 1 || two > 1);") {
        CHECK_TXT_MATCH("one two");

        CHECK_TXT_UNMATCH("one two two");
        CHECK_TXT_UNMATCH("one one two");
        CHECK_TXT_UNMATCH("one one two two");
    }

    // Expected priority: ((one && (!two)) || three)
    TL_TEST(Priority1, "rule test = one && !two || three;") {
        CHECK_TXT_MATCH("one one");
        CHECK_TXT_MATCH("one two three");
        CHECK_TXT_MATCH("one five");
        CHECK_TXT_MATCH("four five three");

        CHECK_TXT_UNMATCH("one two");
        CHECK_TXT_UNMATCH("four five");
    }

    // Expected priority: (one || ((two && three) ^^ (four && five)))
    TL_TEST(Priority2, "rule test = one || two && three ^^ four && five;") {
        CHECK_TXT_MATCH("one one");
        CHECK_TXT_MATCH("four five");
        CHECK_TXT_MATCH("two three");
        CHECK_TXT_MATCH("two four five");
        CHECK_TXT_MATCH("two three five");
        CHECK_TXT_MATCH("one two three four five");

        CHECK_TXT_UNMATCH("two three four five");
        CHECK_TXT_UNMATCH("two four");
        CHECK_TXT_UNMATCH("three five");
        CHECK_TXT_UNMATCH("two five");
        CHECK_TXT_UNMATCH("three four");
        CHECK_TXT_UNMATCH("six");
    }

    TL_TEST(Brace1, "rule test = one && !(two || three);") {
        CHECK_TXT_MATCH("one one");
        CHECK_TXT_MATCH("one four five");

        CHECK_TXT_UNMATCH("one two");
        CHECK_TXT_UNMATCH("one two three");
        CHECK_TXT_UNMATCH("four five");
    }

    TL_TEST(Brace2, "rule test = one && (!two || three);") {
        CHECK_TXT_MATCH("one one");
        CHECK_TXT_MATCH("one two three");
        CHECK_TXT_MATCH("one three");
        CHECK_TXT_MATCH("one four five");

        CHECK_TXT_UNMATCH("one two");
        CHECK_TXT_UNMATCH("four five");
    }

    TL_TEST(OrTrue, "rule test = . || one || two || three || true;") {
        CHECK_TXT_MATCH("one");
        CHECK_TXT_MATCH("two");
        CHECK_TXT_MATCH("three");
        CHECK_TXT_MATCH("four");
    }

    TL_TEST(AndFalse, "rule test = one && false;") {
        CHECK_TXT_UNMATCH("one");
        CHECK_TXT_UNMATCH("two");
    }
}

Y_UNIT_TEST_SUITE(TokenLogicAny) {
    TL_TEST(NoAny1, "rule test = one && !.;") {
        CHECK_TXT_MATCH("one one");
        CHECK_TXT_MATCH("one");

        CHECK_TXT_UNMATCH("one two two");
        CHECK_TXT_UNMATCH("two");
        CHECK_EMPTY_UNMATCH();
    }

    TL_TEST(NoAny2, "rule test = one? && !.;") {
        CHECK_TXT_MATCH("one one");
        CHECK_TXT_MATCH("one");
        CHECK_EMPTY_UNMATCH();

        CHECK_TXT_UNMATCH("one two two");
        CHECK_TXT_UNMATCH("two");
    }

    TL_TEST(OrAny, "rule test = one || .;") {
        CHECK_TXT_MATCH("one one");
        CHECK_TXT_MATCH("one");
        CHECK_TXT_MATCH("one two two");
        CHECK_TXT_MATCH("two");

        CHECK_EMPTY_UNMATCH();
    }
}

Y_UNIT_TEST_SUITE(TokenLogicSyntax) {
    Y_UNIT_TEST(SyntaxError) {
        CHECK_SYNTAX_ERROR("rule test = token? > 0;"); // duplicate operatin ('?' and comparison)
        CHECK_SYNTAX_ERROR("default ;");
        CHECK_SYNTAX_ERROR("use-gzt ;");
        CHECK_SYNTAX_ERROR("rule test = (one && two;");
        CHECK_SYNTAX_ERROR("rule test = ((((one && two)));");
        CHECK_SYNTAX_ERROR("rule test = one && two);");
        CHECK_SYNTAX_ERROR("rule test = one && ;");
        CHECK_SYNTAX_ERROR("rule test = (one &&) two;");
        CHECK_SYNTAX_ERROR("rule test = one || ;");
        CHECK_SYNTAX_ERROR("rule test = one || !;");
        CHECK_SYNTAX_ERROR("rule test = one ^^ ;");
        CHECK_SYNTAX_ERROR("rule test = one ^^ !;");
        CHECK_SYNTAX_ERROR("rule test = token:lab1#gnc(ctx);"); // label must be specified after agreement
        CHECK_SYNTAX_ERROR("rule test = one#dist-(ctx);"); // distance agreement with incorrect length
        CHECK_SYNTAX_ERROR("rule test = one#distnondigit(ctx);"); // distance agreement with incorrect length
        CHECK_SYNTAX_ERROR("rule test = one#dist-3(ctx);"); // distance agreement with incorrect length
        CHECK_SYNTAX_ERROR("token t = token@ttt;"); // invalid simple literal characters
        CHECK_SYNTAX_ERROR("token t = token&token;"); // invalid simple literal characters
        CHECK_SYNTAX_ERROR("token t = token'token;"); // invalid simple literal characters
        CHECK_SYNTAX_ERROR("rule test () = token;"); // invalid weight syntax
        CHECK_SYNTAX_ERROR("rule test (2.3 = token;"); // invalid weight syntax
        CHECK_SYNTAX_ERROR("rule test 2.3) = token;"); // invalid weight syntax
        CHECK_SYNTAX_ERROR("rule test (token) = token;"); // invalid weight syntax
        CHECK_SYNTAX_ERROR("rule test (.) = token;"); // invalid weight syntax
    }

    Y_UNIT_TEST(LogicError) {
        CHECK_SYNTAX_ERROR("rule test = true:label;"); // labeled constant
        CHECK_SYNTAX_ERROR("rule test = false?;"); // constant comparison
        CHECK_SYNTAX_ERROR("rule test = true != 0;"); // constant comparison
        CHECK_SYNTAX_ERROR("rule test = $token;"); // undefined reference
        CHECK_SYNTAX_ERROR("token test = token1; token test = token2;"); // duplicate reference
        CHECK_SYNTAX_ERROR("rule test = .#gnc(ctx);"); // agreement for 'any' token
        CHECK_SYNTAX_ERROR("token any = .; rule test = $any#gnc(ctx);"); // agreement for implicit 'any' token
        CHECK_SYNTAX_ERROR("rule test = token#unknown(ctx);"); // unknown agreement type
        CHECK_SYNTAX_ERROR("rule test = true#gnc(ctx);"); // agreement for constant
    }

    Y_UNIT_TEST(ValidRule) {
        CHECK_SYNTAX_VALID("rule test = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test = [token1 token2];", nullptr);
        CHECK_SYNTAX_VALID("rule test = [*token1 token2];", nullptr);
        CHECK_SYNTAX_VALID("rule test = [?reg=qasq & (reg=wdw | prop=mw)];", nullptr);
        CHECK_SYNTAX_VALID("rule test = token:lab1:lab2:lab3;", nullptr);
        CHECK_SYNTAX_VALID("rule test = token#gnc(ctx1)#no-gzt(ctx2);", nullptr);
        CHECK_SYNTAX_VALID("rule test = token#gnc(ctx1)#no-gzt(ctx2):lab1:lab2;", nullptr);
        CHECK_SYNTAX_VALID("rule test = !token#no-gzt(ctx2):l1?;", nullptr);
        CHECK_SYNTAX_VALID("rule test = !true;", nullptr);
        CHECK_SYNTAX_VALID("rule test = !false;", nullptr);
        CHECK_SYNTAX_VALID("rule test = !!!!!!one;", nullptr);
        CHECK_SYNTAX_VALID("rule test = !!!!!!(one && two);", nullptr);
        CHECK_SYNTAX_VALID("rule test = ((((one)) && (((two)))));", nullptr);
        CHECK_SYNTAX_VALID("rule test = one ^^ two;", nullptr);
        CHECK_SYNTAX_VALID("rule test = one || two ^^ (three && four);", nullptr);
        CHECK_SYNTAX_VALID("rule test = one#dist(ctx);", nullptr); // distance agreement with default length
        CHECK_SYNTAX_VALID("rule test = one#dist3(ctx)#dist1(ctx)#dist(ctx);", nullptr);
        CHECK_SYNTAX_VALID("rule test = one#dist24335(ctx);", nullptr);
        CHECK_SYNTAX_VALID("rule test = one#dist0(ctx);", nullptr);
        CHECK_SYNTAX_VALID("rule test = one#no-dist1(ctx);", nullptr);
    }

    Y_UNIT_TEST(ValidRuleWeighted) {
        CHECK_SYNTAX_VALID("rule test (2) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (2.3) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (2.) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (.3) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (+2) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (+2.3) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (+2.) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (+.3) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (-2) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (-2.3) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (-2.) = token;", nullptr);
        CHECK_SYNTAX_VALID("rule test (-.3) = token;", nullptr);
    }

    Y_UNIT_TEST(ValidToken) {
        CHECK_SYNTAX_VALID("token t = token; rule test = $t;", nullptr);
        CHECK_SYNTAX_VALID("token t1 = token; token t2 = $t1; rule test = $t2;", nullptr);
        CHECK_SYNTAX_VALID("token t1 = token:l1; token t2 = $t1:l2; rule test = $t2:l3;", nullptr);
        CHECK_SYNTAX_VALID("token t1 = token#cn(c1):l1; token t2 = $t1#no-cn(c2):l2; rule test = $t2#cn(c3):l3;", nullptr);
        CHECK_SYNTAX_VALID("token t = \"one3 any text here ^%$#@*';~12\";", nullptr);
        CHECK_SYNTAX_VALID("token t = token-token;", nullptr);
    }

    Y_UNIT_TEST(ValidDefault) {
        CHECK_SYNTAX_VALID("default .;", nullptr);
        CHECK_SYNTAX_VALID("default .:lab:lab2;", nullptr);
        CHECK_SYNTAX_VALID("default !.:lab:lab2?;", nullptr);
        CHECK_SYNTAX_VALID("default (one || !(two && three || !four >= 10) && www<0);", nullptr);
    }

    Y_UNIT_TEST(ValidUseGzt) {
        TGazetteerBuilder buider;
        buider.BuildFromText(
            "import \"base.proto\";"
            "TArticle \"gzt1\" {}"
            "TArticle \"gzt2\" {}"
            );
        THolder<TGazetteer> gzt(buider.MakeGazetteer());
        CHECK_SYNTAX_VALID("use-gzt gzt1;", gzt.Get());
        CHECK_SYNTAX_VALID("use-gzt gzt1, gzt2;", gzt.Get());
    }
}

Y_UNIT_TEST_SUITE(TokenLogicLabel) {
    TL_TEST(Any, "rule test = one:num || .:any;") {
        TWtrokaInputSymbols symbols = ToSymbols("one two three");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "num", "one");
        CHECK_NAMED_TOKENS(res, symbols, "any", "three,two");
    }

    TL_TEST(Multi, "rule test = one:num:num2:num3 || two:lab1:num;") {
        TWtrokaInputSymbols symbols = ToSymbols("one two three");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "num", "one,two");
        CHECK_NAMED_TOKENS(res, symbols, "num2", "one");
        CHECK_NAMED_TOKENS(res, symbols, "num3", "one");
        CHECK_NAMED_TOKENS(res, symbols, "lab1", "two");
    }

    TL_TEST(Ref, "token ref1 = [one two]:num; rule test = $ref1;") {
        TWtrokaInputSymbols symbols = ToSymbols("one two three");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "num", "one,two");
    }

    TL_TEST(RefLabel, "token ref1 = [one two]:num; rule test = $ref1:num2;") {
        TWtrokaInputSymbols symbols = ToSymbols("one two three");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "num", "one,two");
        CHECK_NAMED_TOKENS(res, symbols, "num2", "one,two");
    }

    TL_TEST(MultiRef, "token t1 = one:l1; token t2 = $t1:l2; rule test = $t2:l3;") {
        TWtrokaInputSymbols symbols = ToSymbols("one two three");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "l1", "one");
        CHECK_NAMED_TOKENS(res, symbols, "l2", "one");
        CHECK_NAMED_TOKENS(res, symbols, "l3", "one");
    }

    TL_TEST(Default, "default two:label?; rule test = one;") {
        TWtrokaInputSymbols symbols = ToSymbols("one two three");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "label", "two");

        symbols = ToSymbols("one three");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "label", "");
    }

    TL_TEST(Or, "rule test = one:l || two:l || three:l || .;") {
        TWtrokaInputSymbols symbols = ToSymbols("one");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "l", "one");

        symbols = ToSymbols("two");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "l", "two");

        symbols = ToSymbols("three");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "l", "three");

        symbols = ToSymbols("four");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "l", "");

        symbols = ToSymbols("one two three four");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        // All named tokens must be present in the result independently of possible logic expression optimization
        CHECK_NAMED_TOKENS(res, symbols, "l", "one,three,two");

        symbols = ToSymbols("two three four");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "l", "three,two");
    }

    // See FACTEX-3376
    TL_TEST(Duplicate, "token o1 = one:o1; rule test = $o1 && two || $o1;") {
        TWtrokaInputSymbols symbols = ToSymbols("one two");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        CHECK_NAMED_TOKENS(res, symbols, "o1", "one");
    }
}

Y_UNIT_TEST_SUITE(TokenLogicDefault) {
    TL_TEST(NoAny, "default !.; rule test = one;") {
        CHECK_TXT_MATCH("one one");

        CHECK_TXT_UNMATCH("one two");
    }
    TL_TEST(Several, "default !.; default two > 0; rule test = one;") {
        CHECK_TXT_MATCH("one two");
        CHECK_TXT_MATCH("one one two");
        CHECK_TXT_MATCH("one two two");

        CHECK_TXT_UNMATCH("one one");
        CHECK_TXT_UNMATCH("one three");
        CHECK_TXT_UNMATCH("one two three");
    }

    TL_TEST(Order, "default !.; rule test = one; default two > 0;") {
        CHECK_TXT_MATCH("one one");

        CHECK_TXT_UNMATCH("one two");
        CHECK_TXT_UNMATCH("one one two");
        CHECK_TXT_UNMATCH("one two two");
        CHECK_TXT_UNMATCH("one three");
        CHECK_TXT_UNMATCH("one two three");
    }
}

Y_UNIT_TEST_SUITE(TokenLogicMatching) {
    TL_TEST(Match, "rule test = one && two;") {
        // Implicit "no others" check in Match() method
        CHECK_RESULT(matcher.Match(ToSymbols("one two")));
        CHECK_RESULT(matcher.Match(ToSymbols("one one two one two")));

        CHECK_NO_RESULT(matcher.Match(ToSymbols("one two three")));
        CHECK_NO_RESULT(matcher.Match(ToSymbols("one three")));
    }

    // TODO: fix Match for rules witch 'any' literals
    //TL_TEST(MatchAny, "rule test = one || .;") {
    //    CHECK_RESULT(matcher.Match(ToSymbols("one two")));
    //    CHECK_RESULT(matcher.Match(ToSymbols("one two three")));
    //}

    TL_TEST(SearchAll, "rule test1 = one; rule test2 = two > 1;") {
        CHECK_MATCHED_RULES("one two", "test1");
        CHECK_MATCHED_RULES("one one two one two", "test1,test2");
        CHECK_MATCHED_RULES("two two two", "test2");
    }

    TL_TEST(MatchCache, "rule test1 = one:one1 && two:two; rule test2 = one:one2 && three:three;") {
        // Second rule uses cached result for the "one" token.
        // It should properly calculate "other" tokens and named matches
        TTokenLogicResultPtr res = matcher.Match(ToSymbols("one three"));
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->NamedTokens.size(), 2);
        UNIT_ASSERT_EQUAL(res->NamedTokens.count("one1"), 0);
        UNIT_ASSERT_EQUAL(res->NamedTokens.count("one2"), 1);
        UNIT_ASSERT_EQUAL(res->NamedTokens.count("three"), 1);
    }
}

Y_UNIT_TEST_SUITE(TokenLogicResult) {
    TL_TEST(Single, "rule test = one;") {
        TWtrokaInputSymbols symbols = ToSymbols("one two three");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), 1);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), res->MatchTrack.GetContexts().size());
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, 0)->GetText(), u"one");

        symbols = ToSymbols("two one three");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), 1);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), res->MatchTrack.GetContexts().size());
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, 0)->GetText(), u"one");

        symbols = ToSymbols("one two one");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), 3);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), res->MatchTrack.GetContexts().size());
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, 0)->GetText(), u"one");
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, res->MatchTrack.Size() - 1)->GetText(), u"one");
    }

    TL_TEST(Set, "rule test = [one two];") {
        TTokenLogicResultPtr res = matcher.Search(ToSymbols("one two three"));
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), 2);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), res->MatchTrack.GetContexts().size());

        res = matcher.Search(ToSymbols("one two one"));
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), 3);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), res->MatchTrack.GetContexts().size());
    }

    TL_TEST(ExactNum, "rule test = one == 3;") {
        TTokenLogicResultPtr res = matcher.Search(ToSymbols("one one one"));
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), 3);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), res->MatchTrack.GetContexts().size());
    }

    TL_TEST(Any, "rule test = one && .;") {
        TTokenLogicResultPtr res = matcher.Search(ToSymbols("one two three"));
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), 3);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), res->MatchTrack.GetContexts().size());
    }

    TL_TEST(NoAny, "rule test = [one two] && !.;") {
        TTokenLogicResultPtr res = matcher.Search(ToSymbols("two one one"));
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), 3);
        UNIT_ASSERT_EQUAL(res->MatchTrack.Size(), res->MatchTrack.GetContexts().size());
    }

    TL_TEST(Named, "rule test = one:label;") {
        TWtrokaInputSymbols symbols = ToSymbols("one two three");
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->NamedTokens.size(), 1);
        UNIT_ASSERT_EQUAL(res->NamedTokens.count("label"), 1);
        UNIT_ASSERT_EQUAL(res->NamedTokens.begin()->second, 0);
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, res->NamedTokens.begin()->second)->GetText(), u"one");

        symbols = ToSymbols("two one three");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->NamedTokens.size(), 1);
        UNIT_ASSERT_EQUAL(res->NamedTokens.count("label"), 1);
        // Positions of named tokens are relative to the whole matched range
        UNIT_ASSERT_EQUAL(res->NamedTokens.begin()->second, 0);
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, res->NamedTokens.begin()->second)->GetText(), u"one");

        symbols = ToSymbols("one two one");
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->NamedTokens.size(), 2);
        UNIT_ASSERT_EQUAL(res->NamedTokens.count("label"), 2);
        NSorted::TSimpleMap<TString, size_t>::const_iterator iNamed = res->NamedTokens.begin();
        UNIT_ASSERT_EQUAL(iNamed->second, 0);
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, iNamed->second)->GetText(), u"one");
        ++iNamed;
        UNIT_ASSERT_EQUAL(iNamed->second, 2);
        UNIT_ASSERT_EQUAL(res->GetMatchedSymbol(symbols, iNamed->second)->GetText(), u"one");
    }

    TL_GZT_TEST(Context,
        "import \"base.proto\";"
        "TArticle \"a1\" {key = \"one\";}"
        "TArticle \"a2\" {key = \"one\" | \"two\";}",
        "rule test1 = [?gzt=a1] && \"other\";"
        "rule test2 = [?gzt=a1] && \"three\";"
        "rule test3 = [?gzt=a1,a2] && [?gzt=a2];"
    ) {
        TWtrokaInputSymbols symbols = ToSymbols("one three");
        matcher.ApplyGztResults(symbols, &gzt);
        TTokenLogicResultPtr res = matcher.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->GetMatchedCount(), 2);
        UNIT_ASSERT_EQUAL_C(GetMatchedGzt(res, symbols, 0), TStringBuf("a1"), ", actual: " << GetMatchedGzt(res, symbols, 0));

        symbols = ToSymbols("one two");
        matcher.ApplyGztResults(symbols, &gzt);
        res = matcher.Search(symbols);
        CHECK_RESULT(res);
        UNIT_ASSERT_EQUAL(res->GetMatchedCount(), 2);
        UNIT_ASSERT_EQUAL_C(GetMatchedGzt(res, symbols, 0), TStringBuf("a1,a2"), ", actual: " << GetMatchedGzt(res, symbols, 0));
        UNIT_ASSERT_EQUAL_C(GetMatchedGzt(res, symbols, 1), TStringBuf("a2"), ", actual: " << GetMatchedGzt(res, symbols, 1));
    }
}

Y_UNIT_TEST_SUITE(TokenLogicAgree) {
    TL_GZT_TEST(GztAgreeRef,
        "import \"base.proto\"; TArticle \"art\" {key = \"one\" | \"two\";}",
        "use-gzt art; token t = [one two three four]; rule test = $t#gzt(ctx) > 1;")
    {
        CHECK_GZT_MATCH("one three two");

        CHECK_GZT_UNMATCH("one three");
    }

    TL_GZT_TEST(GztAgreeDef,
        "import \"base.proto\"; TArticle \"art\" {key = \"one\" | \"two\";}",
        "use-gzt art; token t = [one two three four]#gzt(ctx); rule test = $t > 1;")
    {
        CHECK_GZT_MATCH("one three two");

        CHECK_GZT_UNMATCH("one three");
    }

    TL_GZT_TEST(GztAgreeMany,
        "import \"base.proto\"; TArticle \"art\" {key = \"one\" | \"two\";}",
        "use-gzt art; rule test = [one three]#gzt(ctx) && [two four]#gzt(ctx);")
    {
        CHECK_GZT_MATCH("one two");

        CHECK_GZT_UNMATCH("one three");
        CHECK_GZT_UNMATCH("three two");
        CHECK_GZT_UNMATCH("three four");
    }

    // Check complex case when implicit agreement for reference should be used
    TL_GZT_TEST(GztAgreeCopy,
        "import \"base.proto\"; TArticle \"art\" {key = \"one\" | \"two\" | \"three\";}",
        "use-gzt art; token t = one#gzt(ctx1); rule test = $t#gzt(ctx2) && \"two\"#gzt(ctx2) && [three four]#gzt(ctx1);")
    {
        CHECK_GZT_MATCH("one two three");

        CHECK_GZT_UNMATCH("one two four");
    }

    // Check complex case when agreement in one rule should not affect another rule
    TL_GZT_TEST(AgreeForSameToken,
        "import \"base.proto\"; TArticle \"art\" {key = \"one\" | \"two\" | \"three\";}",
        "rule test1 = [?gzt=art]#gzt(ctx) && \"four\"#gzt(ctx);"
        "rule test2 = [?gzt=art] && \"four\";"
    ) {
        CHECK_GZT_MATCHED_RULES("one four", "test2");
    }

    TL_TEST(Distance1Agree, "rule test = one#dist(ctx) && two#dist(ctx);") {
        CHECK_TXT_MATCH("one two");

        CHECK_TXT_UNMATCH("one three two");
    }

    TL_TEST(Distance2Agree, "rule test = \"one\"#dist2(ctx) && \"two\"#dist2(ctx);") {
        CHECK_TXT_MATCH("one two ");
        CHECK_TXT_MATCH("one three two");

        CHECK_TXT_UNMATCH("one three four two");
    }

    // No gaps between tokens
    TL_TEST(DistanceAgreeNoGaps1, "rule test = [one two three four]#dist(ctx) == 4;") {
        CHECK_TXT_MATCH("one two three four");
        CHECK_TXT_MATCH("four one three two");

        CHECK_TXT_UNMATCH("one gap two three four");
        CHECK_TXT_UNMATCH("one two three gap four");
        CHECK_TXT_UNMATCH("one three four gap two");
    }

    // Gaps with not more than 1 token
    TL_TEST(DistanceAgreeNoGaps2, "rule test = [one two three four]#dist2(ctx) == 4;") {
        CHECK_TXT_MATCH("one two three four");
        CHECK_TXT_MATCH("four one three gap two");
        CHECK_TXT_MATCH("one gap two three four");
        CHECK_TXT_MATCH("one gap two gap three gap four");

        CHECK_TXT_UNMATCH("one gap gap two three four");
        CHECK_TXT_UNMATCH("one two three gap gap four");
        CHECK_TXT_UNMATCH("one three four gap gap two");
    }

    // Tokens with gap, which is larger than 1 token
    TL_TEST(NegDistanceAgree, "rule test = \"one\"#no-dist2(ctx) && \"two\"#no-dist2(ctx);") {
        CHECK_TXT_MATCH("one three four two");
        CHECK_TXT_MATCH("one three four five two");

        CHECK_TXT_UNMATCH("one two ");
        CHECK_TXT_UNMATCH("one three two");
    }
}
