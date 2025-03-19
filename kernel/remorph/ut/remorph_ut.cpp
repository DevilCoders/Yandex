#include <kernel/remorph/input/properties.h>
#include <kernel/remorph/input/input_symbol_util.h>
#include <kernel/remorph/matcher/matcher.h>
#include <kernel/remorph/matcher/rule_parser.h>
#include <kernel/remorph/text/textprocessor.h>
#include <kernel/remorph/text/word_input_symbol.h>
#include <kernel/remorph/text/word_symbol_factory.h>
#include <kernel/remorph/tokenizer/tokenizer.h>
#include <kernel/remorph/tokenizer/callback.h>

#include <kernel/gazetteer/gazetteer.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/token/token_structure.h>

#include <util/folder/path.h>
#include <util/folder/tempdir.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/bitmap.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/split.h>
#include <util/string/vector.h>

using namespace NReMorph;
using namespace NText;
using namespace NToken;

const static TUtf16String MAIN_MARKER = u"+";
const static TUtf16String SPACE = u" ";
const static TLangMask TEST_LM = TLangMask(LANG_RUS, LANG_ENG);

inline void ToInput(const TUtf16String& text, TWordInput& input) {
    TWordSymbols symbols = CreateWordSymbols(text, TEST_LM);
    input.Fill(symbols.begin(), symbols.end());
}

inline TWordInputSymbolPtr ToMultiSymbol(const TUtf16String& text) {
    TWordSymbols symbols = CreateWordSymbols(text, TEST_LM);
    return new TWordInputSymbol(symbols.begin(), symbols.end(), TVector<TDynBitMap>(), symbols.size() - 1);
}

TString GetMatchedRules(const TMatchResults& res) {
    TVector<TString> rules;
    for (TMatchResults::const_iterator i = res.begin(); i != res.end(); ++i) {
        rules.push_back(i->Get()->RuleName);
    }
    StableSort(rules.begin(), rules.end());
    rules.erase(Unique(rules.begin(), rules.end()), rules.end());
    return JoinStrings(rules, ",");
}

#define CHECK_SINGLE_MULTIWORD(words, rules) \
        symbols.assign(1, ToMultiSymbol(TUtf16String::FromUtf8(words))); \
        matcher->MatchAll(symbols, res); \
        UNIT_ASSERT_EQUAL_C(GetMatchedRules(res), rules, ": " << GetMatchedRules(res)); \
        res.clear()

#define CHECK_SINGLE_WORD(words, rules) \
        symbols = CreateWordSymbols(TUtf16String::FromUtf8(words), TEST_LM); \
        UNIT_ASSERT_EQUAL_C(symbols.size(), 1, ": got " << symbols.size() << " symbols instead of one"); \
        matcher->MatchAll(symbols, res); \
        UNIT_ASSERT_EQUAL_C(GetMatchedRules(res), rules, ": " << GetMatchedRules(res)); \
        symbols.clear(); \
        res.clear()

#define CHECK_WORD_SEQUENCE(words, rules) \
        symbols = CreateWordSymbols(TUtf16String::FromUtf8(words), TEST_LM); \
        matcher->MatchAll(symbols, res); \
        UNIT_ASSERT_EQUAL_C(GetMatchedRules(res), rules, ": " << GetMatchedRules(res)); \
        symbols.clear(); \
        res.clear()

#define CHECK_WORD_SEQUENCE_INPUT(words, rules) \
        symbols = CreateWordSymbols(TUtf16String::FromUtf8(words), TEST_LM); \
        matcher->CreateInput(input, symbols); \
        matcher->MatchAll(symbols, res); \
        UNIT_ASSERT_EQUAL_C(GetMatchedRules(res), rules, ": " << GetMatchedRules(res)); \
        symbols.clear(); \
        res.clear()

#define CHECK_GZT_WORD_SEQUENCE(words, rules, gzt) \
        symbols = CreateWordSymbols(TUtf16String::FromUtf8(words), TEST_LM); \
        matcher->ApplyGztResults(symbols, gzt); \
        matcher->MatchAll(symbols, res); \
        UNIT_ASSERT_EQUAL_C(GetMatchedRules(res), rules, ": " << GetMatchedRules(res)); \
        symbols.clear(); \
        res.clear()

#define CHECK_AGREE_HEAD(words, def) \
        matcher = TMatcher::Parse(def " rule test = $p#gnc(1) $p#gnc(1);"); \
        CHECK_WORD_SEQUENCE(words, "test")

#define CHECK_SYNTAX_VALID(rules, gzt) UNIT_ASSERT_NO_EXCEPTION(TMatcher::Parse(rules, gzt))

#define CHECK_SYNTAX_INVALID(rules, gzt) UNIT_ASSERT_EXCEPTION(TMatcher::Parse(rules, gzt), yexception)

template <class TInput>
void TestResultBase(TInput& input) {
    TMatcherPtr matcher = TMatcher::Parse("rule test = (<all> (<f> first) (<s> second) (<> third));");
    TMatchResults res;
    matcher->SearchAll(input, res);

    UNIT_ASSERT_EQUAL(res.size(), 1);
    UNIT_ASSERT_EQUAL(res.front()->RuleName, "test");
    UNIT_ASSERT_EQUAL(res.front()->GetMatchedCount(), 3);
    UNIT_ASSERT_EQUAL(res.front()->WholeExpr.first, 1);
    UNIT_ASSERT_EQUAL(res.front()->WholeExpr.second, 4);

    UNIT_ASSERT(res.front()->GetMatchedSymbol(input, 0));
    UNIT_ASSERT_EQUAL(ToString(res.front()->GetMatchedSymbol(input, 0)), "first");
    UNIT_ASSERT(res.front()->GetMatchedSymbol(input, 1));
    UNIT_ASSERT_EQUAL(ToString(res.front()->GetMatchedSymbol(input, 1)), "second");
    UNIT_ASSERT(res.front()->GetMatchedSymbol(input, 2));
    UNIT_ASSERT_EQUAL(ToString(res.front()->GetMatchedSymbol(input, 2)), "third");

    TWordSymbols symbols;
    res.front()->ExtractMatched(input, symbols);
    UNIT_ASSERT_EQUAL(NSymbol::ToString(symbols), "first second third");

    UNIT_ASSERT_UNEQUAL(res.front()->Result->NamedSubmatches.find("f"), res.front()->Result->NamedSubmatches.end());
    symbols.clear();
    res.front()->ExtractMatched(input, res.front()->Result->NamedSubmatches.find("f")->second, symbols);
    UNIT_ASSERT_EQUAL(NSymbol::ToString(symbols), "first");

    UNIT_ASSERT_UNEQUAL(res.front()->Result->NamedSubmatches.find("all"), res.front()->Result->NamedSubmatches.end());
    symbols.clear();
    res.front()->ExtractMatched(input, res.front()->Result->NamedSubmatches.find("all")->second, symbols);
    UNIT_ASSERT_EQUAL(NSymbol::ToString(symbols), "first second third");
}

Y_UNIT_TEST_SUITE(ReMorphText) {
    // Test optimizations for minimal path length in DFA, anchored start state.
    Y_UNIT_TEST(TestDFAMinLength) {
        TWordSymbols symbols;
        TWordInput input;
        TMatcherPtr matcher;
        TMatchResults res;

        ///////////////////////////////////////////////////////////////////
        matcher = TMatcher::Parse("rule test = first second third;");

        symbols.clear();
        symbols = CreateWordSymbols(u"first second third", TEST_LM);
        UNIT_ASSERT(matcher->Match(symbols));
        UNIT_ASSERT(matcher->Search(symbols));

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(0, 2, ToMultiSymbol(u"first second"), false);
        input.CreateBranch(1, 3, ToMultiSymbol(u"second third"), false);
        res.clear();
        matcher->MatchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 1);
        res.clear();
        matcher->SearchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 1);

        symbols.clear();
        symbols = CreateWordSymbols(u"first", TEST_LM);
        UNIT_ASSERT(!matcher->Match(symbols));
        UNIT_ASSERT(!matcher->Search(symbols));

        symbols.clear();
        symbols = CreateWordSymbols(u"zero first second third", TEST_LM);
        UNIT_ASSERT(!matcher->Match(symbols));
        UNIT_ASSERT(matcher->Search(symbols));

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(0, 2, ToMultiSymbol(u"zero first"), false);
        input.CreateBranch(2, 4, ToMultiSymbol(u"second third"), false);
        res.clear();
        matcher->MatchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 0);
        res.clear();
        matcher->SearchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 1);
    }

    Y_UNIT_TEST(TestDFAStartAnchor) {
        TWordSymbols symbols;
        TWordInput input;
        TMatcherPtr matcher;
        TMatchResults res;

        ///////////////////////////////////////////////////////////////////
        matcher = TMatcher::Parse("rule test = ^ first second;");

        symbols.clear();
        symbols = CreateWordSymbols(u"first second", TEST_LM);
        UNIT_ASSERT(matcher->Match(symbols));
        UNIT_ASSERT(matcher->Search(symbols));

        symbols.clear();
        symbols = CreateWordSymbols(u"first", TEST_LM);
        UNIT_ASSERT(!matcher->Match(symbols));
        UNIT_ASSERT(!matcher->Search(symbols));

        symbols.clear();
        symbols = CreateWordSymbols(u"zero first second third", TEST_LM);
        UNIT_ASSERT(!matcher->Match(symbols));
        UNIT_ASSERT(!matcher->Search(symbols));

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(2, 4, ToMultiSymbol(u"second third"), false);
        res.clear();
        matcher->MatchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 0);
        res.clear();
        matcher->SearchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 0);

        ///////////////////////////////////////////////////////////////////
        matcher = TMatcher::Parse("rule test = ^ first second $;");

        symbols.clear();
        symbols = CreateWordSymbols(u"first second", TEST_LM);
        UNIT_ASSERT(matcher->Match(symbols));
        UNIT_ASSERT(matcher->Search(symbols));

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(0, 2, ToMultiSymbol(u"first second"), false);
        res.clear();
        matcher->MatchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 1);
        res.clear();
        matcher->SearchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 1);

        symbols.clear();
        symbols = CreateWordSymbols(u"first", TEST_LM);
        UNIT_ASSERT(!matcher->Match(symbols));
        UNIT_ASSERT(!matcher->Search(symbols));

        symbols.clear();
        symbols = CreateWordSymbols(u"first second third", TEST_LM);
        UNIT_ASSERT(!matcher->Match(symbols));
        UNIT_ASSERT(!matcher->Search(symbols));

        input.Fill(symbols.begin(), symbols.end());
        input.CreateBranch(1, 3, ToMultiSymbol(u"second third"), false);
        res.clear();
        matcher->MatchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 0);
        res.clear();
        matcher->SearchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 0);
    }

    Y_UNIT_TEST(TestFilterOptimization) {
        TGazetteerBuilder buider;
        buider.BuildFromText(
            "import \"base.proto\";"
            "TArticle \"test\" {key=\"!test !test\"}"
            );
        THolder<TGazetteer> gzt(buider.MakeGazetteer());

        TMatcherPtr matcher = TMatcher::Parse("rule r = [?gzt=test];", gzt.Get());
        matcher->SetResolveGazetteerAmbiguity(false);
        matcher->SetFilter(TString("gzt=test"), gzt.Get());

        // ************** Test vector of symbols **********************
        TWordSymbols symbols;
        TMatchResults res;

        symbols = CreateWordSymbols(u"first second", TEST_LM);
        UNIT_ASSERT(!matcher->ApplyGztResults(symbols, gzt.Get()));
        UNIT_ASSERT_EQUAL(matcher->GetSearchableSize(symbols), 0);
        matcher->SearchAll(symbols, res);
        UNIT_ASSERT_EQUAL(res.size(), 0);

        symbols.clear();
        res.clear();
        symbols = CreateWordSymbols(u"first second test test", TEST_LM);
        UNIT_ASSERT(matcher->ApplyGztResults(symbols, gzt.Get()));
        UNIT_ASSERT_EQUAL(matcher->GetSearchableSize(symbols), 3);
        matcher->SearchAll(symbols, res);
        UNIT_ASSERT_EQUAL(res.size(), 1);

        symbols.clear();
        res.clear();
        symbols = CreateWordSymbols(u"test test first second", TEST_LM);
        UNIT_ASSERT(matcher->ApplyGztResults(symbols, gzt.Get()));
        UNIT_ASSERT_EQUAL(matcher->GetSearchableSize(symbols), 1);
        matcher->SearchAll(symbols, res);
        UNIT_ASSERT_EQUAL(res.size(), 1);

        // ************** Test structured input **********************
        TWordInput input;

        symbols.clear();
        res.clear();
        symbols = CreateWordSymbols(u"first second", TEST_LM);
        UNIT_ASSERT(!matcher->CreateInput(input, symbols, gzt.Get()));
        matcher->SearchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 0);

        input.Clear();
        symbols.clear();
        res.clear();
        symbols = CreateWordSymbols(u"test test first second", TEST_LM);
        UNIT_ASSERT(matcher->CreateInput(input, symbols, gzt.Get()));
        matcher->SearchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 1);

        input.Clear();
        symbols.clear();
        res.clear();
        symbols = CreateWordSymbols(u"first test test second", TEST_LM);
        UNIT_ASSERT(matcher->CreateInput(input, symbols, gzt.Get()));
        matcher->SearchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 1);

        input.Clear();
        symbols.clear();
        res.clear();
        symbols = CreateWordSymbols(u"first test test test second", TEST_LM);
        UNIT_ASSERT(matcher->CreateInput(input, symbols, gzt.Get()));
        matcher->SearchAll(input, res);
        UNIT_ASSERT_EQUAL(res.size(), 2);
    }

    Y_UNIT_TEST(TestInputResult) {
        TWordInput input;
        ToInput(u"zero first second third", input);
        input.CreateBranch(0, 2, ToMultiSymbol(u"zero first"), false);
        input.CreateBranch(2, 4, ToMultiSymbol(u"second third"), false);
        TestResultBase(input);
    }

    Y_UNIT_TEST(TestVectorResult) {
        TWordSymbols symbols = CreateWordSymbols(u"zero first second third", TEST_LM);
        TestResultBase(symbols);
    }

    Y_UNIT_TEST(TestRepeatedDefSubmatch) {
        TString rules =
            "def num = (<num>[?prop=num]);\n"
            "rule repeated_num = $num $num;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TMatchResultPtr res = matcher->Search(CreateWordSymbols(u"123 456", TEST_LM));
        UNIT_ASSERT(res);
        UNIT_ASSERT_EQUAL(res->GetMatchedCount(), 2);
        UNIT_ASSERT_EQUAL(res->Result->NamedSubmatches.size(), 2);
        UNIT_ASSERT_EQUAL(res->Result->NamedSubmatches.count("num"), 2);
    }

    Y_UNIT_TEST(TestTextCases) {
        TString rules =
            "rule camel = ^ (<camel>[? prop=cs-camel]) $;\n"
            "rule lower = ^ (<lower>[? prop=cs-lower]) $;\n"
            "rule mixed = ^ (<mixed>[? prop=cs-mixed]) $;\n"
            "rule title = ^ (<title>[? prop=cs-title]) $;\n"
            "rule upper = ^ (<upper>[? prop=cs-upper]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("lower", "lower");
        CHECK_SINGLE_WORD("UPPER", "upper");
        CHECK_SINGLE_WORD("Title", "title");
        CHECK_SINGLE_WORD("mIxEd", "mixed");
        CHECK_SINGLE_WORD("mIXED", "mixed");
        CHECK_SINGLE_WORD("MixeD", "mixed");

        CHECK_SINGLE_WORD("123", "");

        // Multitokens.
        CHECK_SINGLE_WORD("08.09.2013", "");
        CHECK_SINGLE_WORD("U'PPER", "upper");
        CHECK_SINGLE_WORD("UP'PER", "upper");
        CHECK_SINGLE_WORD("T'Itle", "camel,title");
        CHECK_SINGLE_WORD("T'title", "title");
        CHECK_SINGLE_WORD("MI'Xed", "mixed");
        CHECK_SINGLE_WORD("m'Ixed", "mixed");
        CHECK_SINGLE_WORD("mI'Xed", "mixed");
        CHECK_SINGLE_WORD("mI'xed", "mixed");
        CHECK_SINGLE_WORD("mI'XED", "mixed");
        CHECK_SINGLE_WORD("mixe'D", "mixed");
        CHECK_SINGLE_WORD("Came'L", "camel,title");
        CHECK_SINGLE_WORD("Cam'E'L", "camel,title");
        CHECK_SINGLE_WORD("Ca'M'El", "camel,title");
        CHECK_SINGLE_WORD("C'A'M'E'L", "camel,title,upper");

        // Multiwords.
        CHECK_SINGLE_MULTIWORD("two words", "lower");
        CHECK_SINGLE_MULTIWORD("two Words", "mixed");
        CHECK_SINGLE_MULTIWORD("two WORDS", "mixed");
        CHECK_SINGLE_MULTIWORD("two wORDs", "mixed");
        CHECK_SINGLE_MULTIWORD("Two words", "title");
        CHECK_SINGLE_MULTIWORD("Two Words", "camel,title");
        CHECK_SINGLE_MULTIWORD("Two WORDS", "mixed");
        CHECK_SINGLE_MULTIWORD("Two wORDs", "mixed");
        CHECK_SINGLE_MULTIWORD("TWO words", "mixed");
        CHECK_SINGLE_MULTIWORD("TWO Words", "mixed");
        CHECK_SINGLE_MULTIWORD("TWO WORDS", "upper");
        CHECK_SINGLE_MULTIWORD("TWO wORDs", "mixed");
        CHECK_SINGLE_MULTIWORD("tWO words", "mixed");
        CHECK_SINGLE_MULTIWORD("tWO Words", "mixed");
        CHECK_SINGLE_MULTIWORD("tWO WORDS", "mixed");
        CHECK_SINGLE_MULTIWORD("tWO wORDs", "mixed");
        CHECK_SINGLE_MULTIWORD("a b", "lower");
        CHECK_SINGLE_MULTIWORD("a B", "mixed");
        CHECK_SINGLE_MULTIWORD("A b", "title");
        CHECK_SINGLE_MULTIWORD("A B", "camel,title,upper");
        CHECK_SINGLE_MULTIWORD("Three more Words", "camel,title");
        CHECK_SINGLE_MULTIWORD("Three More Words", "camel,title");
        CHECK_SINGLE_MULTIWORD("Three More words", "camel,title");
        CHECK_SINGLE_MULTIWORD("Three More WORDS", "mixed");
        CHECK_SINGLE_MULTIWORD("three more Words", "mixed");
    }

    Y_UNIT_TEST(TestTextCaseFirstUpper) {
        TString rules =
            "rule f_upper = ^ (<f_upper>[? prop=cs-1upper]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "");
        CHECK_SINGLE_WORD("Squirrel", "f_upper");
        CHECK_SINGLE_WORD("SQUIRREL", "f_upper");
        CHECK_SINGLE_WORD("sQUIrrel", "");
        CHECK_SINGLE_WORD("SQUIrrel", "f_upper");
        CHECK_SINGLE_WORD("белка", "");
        CHECK_SINGLE_WORD("Белка", "f_upper");
        CHECK_SINGLE_WORD("БЕЛКА", "f_upper");
        CHECK_SINGLE_WORD("бЕЛка", "");
        CHECK_SINGLE_WORD("БЕЛка", "f_upper");
        CHECK_SINGLE_WORD("459", "");

        CHECK_SINGLE_WORD("s", "");
        CHECK_SINGLE_WORD("S", "f_upper");
        CHECK_SINGLE_WORD("б", "");
        CHECK_SINGLE_WORD("Б", "f_upper");
        CHECK_SINGLE_WORD("4", "");

        CHECK_SINGLE_WORD("meta-squirrel", "");
        CHECK_SINGLE_WORD("meta-Squirrel", "");
        CHECK_SINGLE_WORD("Meta-squirrel", "f_upper");
        CHECK_SINGLE_WORD("Meta-Squirrel", "f_upper");
        CHECK_SINGLE_WORD("мета-белка", "");
        CHECK_SINGLE_WORD("мета-Белка", "");
        CHECK_SINGLE_WORD("Мета-белка", "f_upper");
        CHECK_SINGLE_WORD("Мета-Белка", "f_upper");

        CHECK_SINGLE_MULTIWORD("meta squirrel", "");
        CHECK_SINGLE_MULTIWORD("meta Squirrel", "");
        CHECK_SINGLE_MULTIWORD("Meta squirrel", "f_upper");
        CHECK_SINGLE_MULTIWORD("Meta Squirrel", "f_upper");
        CHECK_SINGLE_MULTIWORD("мета белка", "");
        CHECK_SINGLE_MULTIWORD("мета Белка", "");
        CHECK_SINGLE_MULTIWORD("Мета белка", "f_upper");
        CHECK_SINGLE_MULTIWORD("Мета Белка", "f_upper");
    }

    Y_UNIT_TEST(TestTextCategories) {
        TString rules =
            "rule alpha =   ^ (<alpha>[? prop=alpha]) $;\n"
            "rule num =     ^ (<num>[? prop=num]) $;\n"
            "rule ascii =   ^ (<ascii>[? prop=ascii]) $;\n"
            "rule nascii =  ^ (<nascii>[? prop=nascii]) $;\n"
            "rule nmtoken = ^ (<nmtoken>[? prop=nmtoken]) $;\n"
            "rule nutoken = ^ (<nutoken>[? prop=nutoken]) $;\n"
            "rule cmp =     ^ (<cmp>[? prop=cmp]) $;\n"
            "rule calpha =  ^ (<alpha>[? prop=calpha]) $;\n"
            "rule cnum =    ^ (<cnum>[? prop=cnum]) $;\n"
            "rule mw =      ^ (<mw>[? prop=mw]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "alpha,ascii,calpha,nmtoken");
        CHECK_SINGLE_WORD("459", "ascii,cnum,num,nutoken");
        CHECK_SINGLE_WORD("белка", "alpha,calpha,nascii,nmtoken");
        CHECK_SINGLE_WORD("squirrel459", "ascii,cmp,nmtoken");
        CHECK_SINGLE_WORD("белка459", "cmp,nmtoken");
        CHECK_SINGLE_WORD("459squirrel", "ascii,cmp,nutoken");
        CHECK_SINGLE_WORD("459белка", "cmp,nutoken");

        CHECK_SINGLE_WORD("meta-squirrel", "ascii,calpha,cmp");
        CHECK_SINGLE_WORD("мета-белка", "calpha,cmp");
        CHECK_SINGLE_WORD("meta-459", "ascii,cmp");
        CHECK_SINGLE_WORD("мета-459", "cmp");
        CHECK_SINGLE_WORD("459-squirrel", "ascii,cmp");
        CHECK_SINGLE_WORD("459-белка", "cmp");
        CHECK_SINGLE_WORD("123-459", "ascii,cmp,cnum");
        CHECK_SINGLE_WORD("д'Артаньян", "calpha,cmp");
        CHECK_SINGLE_WORD("д’Артаньян", "calpha,cmp,nascii");

        CHECK_SINGLE_MULTIWORD("meta squirrel", "ascii,calpha,cmp,mw");
        CHECK_SINGLE_MULTIWORD("мета белка", "calpha,cmp,mw");
        CHECK_SINGLE_MULTIWORD("meta 459", "ascii,cmp,mw");
        CHECK_SINGLE_MULTIWORD("мета 459", "cmp,mw");
        CHECK_SINGLE_MULTIWORD("459 squirrel", "ascii,cmp,mw");
        CHECK_SINGLE_MULTIWORD("459 белка", "cmp,mw");
        CHECK_SINGLE_MULTIWORD("123 459", "ascii,cmp,cnum,mw");
        CHECK_SINGLE_MULTIWORD("big meta-squirrel", "ascii,calpha,cmp,mw");
        CHECK_SINGLE_MULTIWORD("123 567-459", "ascii,cmp,cnum,mw");
        CHECK_SINGLE_MULTIWORD("большая мета-белка", "calpha,cmp,mw");
        CHECK_SINGLE_MULTIWORD("В И Ленин", "calpha,cmp,mw");
        CHECK_SINGLE_MULTIWORD("В.И. Ленин", "cmp,mw");
        CHECK_SINGLE_MULTIWORD("ул Льва Толстого", "calpha,cmp,mw");
        CHECK_SINGLE_MULTIWORD("ул. Льва Толстого", "cmp,mw");
    }

    Y_UNIT_TEST(TestFirstLast) {
        TString rules =
            "rule first = ^ (<first> [? prop=first]) $;\n"
            "rule last = ^ (<last> [? prop=last]) $;\n"
            "rule first_last = ^ (<first_last> [? prop=first,last]) $;\n"
            ;
        TString rulesForSequence =
            "rule first_to_last = ^ (<first_to_last> [? (prop=first) & (prop!=last)] [? prop!=first,last] * [? (prop!=first) & (prop=last)]) $;\n"
            ;

        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "first,first_last,last");
        CHECK_SINGLE_MULTIWORD("meta squirrel", "first,first_last,last");

        matcher = TMatcher::Parse(rulesForSequence);

        CHECK_WORD_SEQUENCE("a b c d e", "first_to_last");

        NRemorph::TInput<TWordInputSymbolPtr> input;

        CHECK_WORD_SEQUENCE_INPUT("a b c d e", "first_to_last");
    }

    Y_UNIT_TEST(TestFirstLastInput) {
        TString rulesForSequence =
            "rule first_to_last = ^ (<first_to_last> [? (prop=first) & (prop!=last)] [? prop!=first,last] * [? (prop!=first) & (prop=last)]) $;\n"
            ;

        TMatcherPtr matcher = TMatcher::Parse(rulesForSequence);
        TWordSymbols symbols;
        TMatchResults res;
        TWordInput input;

        CHECK_WORD_SEQUENCE_INPUT("a b c d e", "first_to_last");
    }

    Y_UNIT_TEST(TestAgree) {
        TString rules =
            "rule c = [?prop!=mw]#c(1) [?prop!=mw]#c(1);\n"
            "rule n = [?prop!=mw]#n(1) [?prop!=mw]#n(1);\n"
            "rule cn = [?prop!=mw]#cn(1) [?prop!=mw]#cn(1);\n"
            "rule gnc = [?prop!=mw]#gnc(1) [?prop!=mw]#gnc(1);\n"
            "rule no_c = [?prop!=mw]#no-c(1) [?prop!=mw]#no-c(1);\n"
            "rule no_n = [?prop!=mw]#no-n(1) [?prop!=mw]#no-n(1);\n"
            "rule no_cn = [?prop!=mw]#no-cn(1) [?prop!=mw]#no-cn(1);\n"
            "rule no_gnc = [?prop!=mw]#no-gnc(1) [?prop!=mw]#no-gnc(1);\n"
            ;

        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_WORD_SEQUENCE("красивый дом", "c,cn,gnc,n");
        CHECK_WORD_SEQUENCE("красивых домов", "c,cn,gnc,n");
        CHECK_WORD_SEQUENCE("красивых дом", "c,no_cn,no_gnc,no_n");
        CHECK_WORD_SEQUENCE("красивого доме", "n,no_c,no_cn,no_gnc");
    }

    Y_UNIT_TEST(TestAgreeAmbig) {
        TString rules =
            "rule r01 = ([?gram=A]#gnc(1))+ [?gram=S]#gnc(1);\n"
            "rule r02 = ([?gram=A,gen]#gnc(1))+ [?gram=S]#gnc(1);\n"
            "rule r03 = ([?gram=A,nom]#gnc(1))+ [?gram=S]#gnc(1);\n"

            "rule r11 = ([?lang=rus]#gnc(1))+ [?gram=S]#gnc(1);\n"

            "rule r21 = ([?gram=A]#gnc(1))+ [?lang=rus]#gnc(1);\n"
            "rule r22 = ([?gram=A,gen]#gnc(1))+ [?lang=rus]#gnc(1);\n"
            "rule r23 = ([?gram=A,nom]#gnc(1))+ [?lang=rus]#gnc(1);\n"
            ;

        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_WORD_SEQUENCE("большой дом", "r01,r03,r11,r21,r23");
        CHECK_WORD_SEQUENCE("большой энциклопедии", "r01,r02,r11,r21,r22");
        CHECK_WORD_SEQUENCE("большой советской энциклопедии", "r01,r02,r11,r21,r22");
        CHECK_WORD_SEQUENCE("большой советский энциклопедии", "");
        CHECK_WORD_SEQUENCE("большой советский энциклопедия", "");
        CHECK_WORD_SEQUENCE("большой красивый дом", "r01,r03,r11,r21,r23");
        CHECK_WORD_SEQUENCE("большой красивой дом", "");
        CHECK_WORD_SEQUENCE("большой красивой домов", "");
    }

    // When macros can match multiple words, test that agreement between macros works as agreement between main words
    Y_UNIT_TEST(TestAgreeHead) {
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw] [? prop!=mw]; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw] [? prop!=mw] | [? prop!=mw]; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]?; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]+; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]*; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]* [? prop!=mw]; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]#head | [? prop!=mw]; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]#head | [? prop!=mw]#head | [? prop!=mw]; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]#head?; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]#head+; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]#head*; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = ([? prop!=mw]#head [? prop!=mw])*; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse("def a = [? prop!=mw]#head? [? prop!=mw]; rule test = $a#c(1) $a#c(1);"), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse(
            "def n = \"a\" \"b\"#head;"
            "def a1 = \"c\" [?gram=S]#head;"
            "def a2 = \"d\" [?gram=S]#head;"
            "def a = $a1 | $a2;"
            "def p = $n $a; rule test = $p#c(1) $p#c(1);"
        ), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse(
            "def n = \"a\" \"b\"#head;"
            "def a1 = \"c\" [?gram=S]#head;"
            "def a2 = \"d\" [?gram=S]#head;"
            "def a = $a1 | $a2;"
            "def p = $n? $a; rule test = $p#c(1) $p#c(1);"
        ), yexception);
        UNIT_ASSERT_EXCEPTION(TMatcher::Parse(
            "def n = \"a\" \"b\"#head;"
            "def a1 = \"c\" [?gram=S];"
            "def a2 = \"d\" [?gram=S];"
            "def a = $a1 | $a2;"
            "def p = $n? $a; rule test = $p#c(1) $p#c(1);"
        ), yexception);

        TWordSymbols symbols;
        TMatchResults res;
        TMatcherPtr matcher;

        CHECK_AGREE_HEAD("красивый дом красный забор", "def p = [?gram=A] [?gram=S]#head;");
        CHECK_AGREE_HEAD("красивый дом забор", "def p = [?gram=A]* [?gram=S]#head;");
        CHECK_AGREE_HEAD("красивый дом красный", "def p = [?gram=A] [?gram=S]#head | [?gram=A]#head | [?gram=abbr]#head;");
        CHECK_AGREE_HEAD("очень красивый дом очень красный забор",
            "def a1 = \"красивый\" [?gram=S]#head; \n"
            "def a2 = \"красный\" [?gram=S]#head; \n"
            "def a = $a1 | $a2; \n"
            "def p = \"очень\" $a#head;"
        );
        CHECK_AGREE_HEAD("не очень красивый дом не очень красный забор",
            "def n = \"не\" \"очень\"#head; \n"
            "def a1 = \"красивый\" [?gram=S]#head; \n"
            "def a2 = \"красный\" [?gram=S]#head; \n"
            "def a = $a1 | $a2; \n"
            "def p = $n $a#head;"
        );
        CHECK_AGREE_HEAD("красивый дом красный забор",
            "def n = \"не\" \"очень\"#head; \n"
            "def a1 = \"красивый\" [?gram=S]#head; \n"
            "def a2 = \"красный\" [?gram=S]#head; \n"
            "def a = $a1 | $a2; \n"
            "def p = $n? $a#head;"
        );
        CHECK_AGREE_HEAD("красивый дом красный забор",
            "def n = \"не\" \"очень\"; \n"
            "def a1 = \"красивый\" [?gram=S]#head; \n"
            "def a2 = \"красный\" [?gram=S]#head; \n"
            "def a = $a1 | $a2; \n"
            "def p = $n? $a;"
        );
    }

    Y_UNIT_TEST(TestDistanceAgree) {
        TString rules =
            "rule any = one .* two;\n"
            "rule d1 = one#dist(ctx) .* two#dist(ctx);\n"
            "rule d2 = one#dist2(ctx) .* two#dist2(ctx);\n"
            "rule nd1 = one#no-dist(ctx) .* two#no-dist(ctx);\n"
            "rule nd2 = one#no-dist2(ctx) .* two#no-dist2(ctx);\n"
            ;

        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_WORD_SEQUENCE("one two", "any,d1,d2");
        CHECK_WORD_SEQUENCE("one gap two", "any,d2,nd1");
        CHECK_WORD_SEQUENCE("one gap gap two", "any,nd1,nd2");
        CHECK_WORD_SEQUENCE("one gap gap gap two", "any,nd1,nd2");
    }

    Y_UNIT_TEST(TestAgreeMark) {
        TString rules =
            "rule r1 = . /#gnc(1) .;\n"
            "rule r2 = [?gram=A] /#gnc(1) [?gram=S];\n"
            "rule r3 = /#gnc(1) . . /#gnc(1);\n"
            "rule r4 = [?gram=A]#gnc(1) [?gram=S] /#gnc(1);\n"
            "rule r5 = /#gnc(1) [?gram=A] [?gram=S]#gnc(1);\n"
            ;

        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_WORD_SEQUENCE("большой дом", "r1,r2,r3,r4,r5");
        CHECK_WORD_SEQUENCE("большой домов", "");
    }

    Y_UNIT_TEST(TestTextAgree) {
        TString rules =
            "def tok = [one two three];\n"
            "rule r1 = $tok#txt(ctx) $tok#txt(ctx);\n"
            "rule r2 = $tok#no-txt(ctx) $tok#no-txt(ctx);\n"
            "rule r3 = $tok $tok;\n"
            ;

        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_WORD_SEQUENCE("one two", "r2,r3");
        CHECK_WORD_SEQUENCE("one one", "r1,r3");
        CHECK_WORD_SEQUENCE("two two", "r1,r3");
    }

    Y_UNIT_TEST(TestGztIdAgree) {
        const TString rules =
            "def tok = [?gzt=one,two];\n"
            "rule r1 = $tok#gztid(ctx) $tok#gztid(ctx);\n"
            "rule r2 = $tok#no-gztid(ctx) $tok#no-gztid(ctx);\n"
            "rule r3 = $tok $tok;\n"
            ;

        const TString gztSrc =
            "import \"base.proto\";"
            "TArticle \"one\" {key=[\"!one\" | \"1\"]}"
            "TArticle \"two\" {key=[\"!two\" | \"2\"]}"
            ;
        TGazetteerBuilder buider;
        buider.BuildFromText(gztSrc);
        THolder<TGazetteer> gzt(buider.MakeGazetteer());
        TMatcherPtr matcher = TMatcher::Parse(rules, gzt.Get());

        TWordSymbols symbols;
        TMatchResults res;

        CHECK_GZT_WORD_SEQUENCE("one two", "r2,r3", gzt.Get());
        CHECK_GZT_WORD_SEQUENCE("1 two", "r2,r3", gzt.Get());
        CHECK_GZT_WORD_SEQUENCE("1 2", "r2,r3", gzt.Get());
        CHECK_GZT_WORD_SEQUENCE("one one", "r1,r3", gzt.Get());
        CHECK_GZT_WORD_SEQUENCE("one 1", "r1,r3", gzt.Get());
        CHECK_GZT_WORD_SEQUENCE("2 two", "r1,r3", gzt.Get());
        CHECK_GZT_WORD_SEQUENCE("two two", "r1,r3", gzt.Get());
    }

    // Check case when agreement of one literal should not affect another rule
    Y_UNIT_TEST(TestAgreeAffection) {
        const TString gztSrc =
            "import \"base.proto\";"
            "TArticle \"art\" {key = \"one\" | \"two\" | \"three\";}"
            ;

        const TString rules =
            "rule r1 = [?gzt=art]#gzt(ctx) \"four\"#gzt(ctx);"
            "rule r2 = [?gzt=art] \"four\";"
            ;

        TGazetteerBuilder buider;
        buider.BuildFromText(gztSrc);
        THolder<TGazetteer> gzt(buider.MakeGazetteer());
        TMatcherPtr matcher = TMatcher::Parse(rules, gzt.Get());

        TWordSymbols symbols;
        TMatchResults res;

        CHECK_GZT_WORD_SEQUENCE("one four", "r2", gzt.Get());
    }

    Y_UNIT_TEST(TestSimpleLiterals) {
        TString rules =
            "rule squirrel_eng =                      ^ (<m> squirrel) $;\n"
            "rule squirrel_rus =                      ^ (<m> белка) $;\n"
            "rule number_456 =                        ^ (<m> 456) $;\n"
            "rule d_artagnan_rus_apos_unicode_alpha = ^ (<m> дʼартаньян) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_WORD_SEQUENCE("squirrel", "squirrel_eng");
        CHECK_WORD_SEQUENCE("белка", "squirrel_rus");
        CHECK_WORD_SEQUENCE("456", "number_456");
        CHECK_WORD_SEQUENCE("дʼАртаньян", "d_artagnan_rus_apos_unicode_alpha");
    }

    Y_UNIT_TEST(TestSimpleLiteralsQuoted) {
        TString rules =
            "rule squirrel_eng =                      ^ (<m> \"squirrel\") $;\n"
            "rule squirrel_rus =                      ^ (<m> \"белка\") $;\n"
            "rule number_456 =                        ^ (<m> \"456\") $;\n"
            "rule dot =                               ^ (<m> \".\") $;\n"
            "rule comma =                             ^ (<m> \",\") $;\n"
            "rule d_artagnan_rus_apos_ascii =         ^ (<m> \"д'артаньян\") $;\n"
            "rule d_artagnan_rus_apos_unicode_alpha = ^ (<m> \"дʼартаньян\") $;\n"
            "rule d_artagnan_rus_apos_unicode_punct = ^ (<m> \"д’артаньян\") $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel_eng");
        CHECK_SINGLE_WORD("белка", "squirrel_rus");
        CHECK_SINGLE_WORD("456", "number_456");
        CHECK_SINGLE_WORD(".", "dot");
        CHECK_SINGLE_WORD(",", "comma");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan_rus_apos_ascii");
        CHECK_SINGLE_WORD("дʼАртаньян", "d_artagnan_rus_apos_unicode_alpha");
        CHECK_SINGLE_WORD("д’Артаньян", "d_artagnan_rus_apos_unicode_punct");
    }

    Y_UNIT_TEST(TestLiteralSet) {
        TString rules =
            "rule squirrel =   ^ (<m> [squirrel белка]) $;\n"
            "rule number_456 = ^ (<m> [456]) $;\n"
            "rule d_artagnan = ^ (<m> [дʼартаньян]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel");
        CHECK_SINGLE_WORD("белка", "squirrel");
        CHECK_SINGLE_WORD("456", "number_456");
        CHECK_SINGLE_WORD("дʼАртаньян", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralSetQuoted) {
        TString rules =
            "rule squirrel =   ^ (<m> [\"squirrel\" \"белка\"]) $;\n"
            "rule number_456 = ^ (<m> [\"456\"]) $;\n"
            "rule punct =      ^ (<m> [\".\" \",\"]) $;\n"
            "rule d_artagnan = ^ (<m> [\"д'артаньян\" дʼартаньян \"д’артаньян\"]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel");
        CHECK_SINGLE_WORD("белка", "squirrel");
        CHECK_SINGLE_WORD("456", "number_456");
        CHECK_SINGLE_WORD(".", "punct");
        CHECK_SINGLE_WORD(",", "punct");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼАртаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’Артаньян", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralLogicText) {
        TString rules =
            "rule squirrel_eng = ^ (<m> [? text=squirrel]) $;\n"
            "rule squirrel_eng_upper = ^ (<m> [? text=SQUIRREL]) $;\n"
            "rule squirrel_rus = ^ (<m> [? text=белка]) $;\n"
            "rule squirrel_rus_upper = ^ (<m> [? text=БЕЛКА]) $;\n"
            "rule number_456 = ^ (<m> [? text=456]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel_eng");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel_eng_upper");
        CHECK_SINGLE_WORD("белка", "squirrel_rus");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel_rus_upper");
        CHECK_SINGLE_WORD("456", "number_456");
    }

    Y_UNIT_TEST(TestLiteralLogicTextQuoted) {
        TString rules =
            "rule squirrel_eng = ^ (<m> [? text=\"squirrel\"]) $;\n"
            "rule squirrel_eng_upper = ^ (<m> [? text=\"SQUIRREL\"]) $;\n"
            "rule squirrel_rus = ^ (<m> [? text=\"белка\"]) $;\n"
            "rule squirrel_rus_upper = ^ (<m> [? text=\"БЕЛКА\"]) $;\n"
            "rule number_456 = ^ (<m> [? text=\"456\"]) $;\n"
            "rule d_artagnan = ^ (<m> [? text=\"д'Артаньян\"]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel_eng");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel_eng_upper");
        CHECK_SINGLE_WORD("белка", "squirrel_rus");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel_rus_upper");
        CHECK_SINGLE_WORD("456", "number_456");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralLogicTextRe) {
        TString rules =
            "rule squirrel = ^ (<m> [? text=/squirrel|белка/]) $;\n"
            "rule squirrel_upper = ^ (<m> [? text=/SQUIRREL|БЕЛКА/]) $;\n"
            "rule number = ^ (<m> [? text=/\\d+/]) $;\n"
            "rule d_artagnan = ^ (<m> [? text=/д('|ʼ|’)Артаньян/]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel_upper");
        CHECK_SINGLE_WORD("белка", "squirrel");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel_upper");
        CHECK_SINGLE_WORD("456", "number");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼАртаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’Артаньян", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralLogicTextReI) {
        TString rules =
            "rule squirrel = ^ (<m> [? text=/squirrel|белка/i]) $;\n"
            "rule d_artagnan = ^ (<m> [? text=/д('|ʼ|’)Артаньян/i]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel");
        CHECK_SINGLE_WORD("белка", "squirrel");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼАртаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д'артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼартаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("Д'АРТАНЬЯН", "d_artagnan");
        CHECK_SINGLE_WORD("ДʼАРТАНЬЯН", "d_artagnan");
        CHECK_SINGLE_WORD("Д’АРТАНЬЯН", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralLogicTextReCompat) {
        TString rules =
            "rule squirrel = ^ (<m> [? reg=\"squirrel|белка\"]) $;\n"
            "rule squirrel_upper = ^ (<m> [? reg=\"SQUIRREL|БЕЛКА\"]) $;\n"
            "rule number = ^ (<m> [? reg=\"\\d+\"]) $;\n"
            "rule d_artagnan = ^ (<m> [? reg=\"д('|ʼ|’)Артаньян\"]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel_upper");
        CHECK_SINGLE_WORD("белка", "squirrel");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel_upper");
        CHECK_SINGLE_WORD("456", "number");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼАртаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’Артаньян", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralLogicTextReICompat) {
        TString rules =
            "rule squirrel = ^ (<m> [? ireg=\"squirrel|белка\"]) $;\n"
            "rule d_artagnan = ^ (<m> [? ireg=\"д('|ʼ|’)Артаньян\"]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel");
        CHECK_SINGLE_WORD("белка", "squirrel");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼАртаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д'артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼартаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("Д'АРТАНЬЯН", "d_artagnan");
        CHECK_SINGLE_WORD("ДʼАРТАНЬЯН", "d_artagnan");
        CHECK_SINGLE_WORD("Д’АРТАНЬЯН", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralLogicTextNorm) {
        TString rules =
            "rule squirrel_eng = ^ (<m> [? ntext=squirrel]) $;\n"
            "rule squirrel_rus = ^ (<m> [? ntext=белка]) $;\n"
            "rule number_456 = ^ (<m> [? ntext=456]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel_eng");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel_eng");
        CHECK_SINGLE_WORD("Squirrel", "squirrel_eng");
        CHECK_SINGLE_WORD("белка", "squirrel_rus");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel_rus");
        CHECK_SINGLE_WORD("Белка", "squirrel_rus");
        CHECK_SINGLE_WORD("456", "number_456");
    }

    Y_UNIT_TEST(TestLiteralLogicTextNormQuoted) {
        TString rules =
            "rule squirrel_eng = ^ (<m> [? ntext=\"squirrel\"]) $;\n"
            "rule squirrel_rus = ^ (<m> [? ntext=\"белка\"]) $;\n"
            "rule number_456 = ^ (<m> [? ntext=\"456\"]) $;\n"
            "rule d_artagnan = ^ (<m> [? ntext=\"д'артаньян\"]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel_eng");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel_eng");
        CHECK_SINGLE_WORD("Squirrel", "squirrel_eng");
        CHECK_SINGLE_WORD("белка", "squirrel_rus");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel_rus");
        CHECK_SINGLE_WORD("Белка", "squirrel_rus");
        CHECK_SINGLE_WORD("456", "number_456");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralLogicTextNormRe) {
        TString rules =
            "rule squirrel = ^ (<m> [? ntext=/squirrel|белка/]) $;\n"
            "rule number = ^ (<m> [? ntext=/\\d+/]) $;\n"
            "rule d_artagnan = ^ (<m> [? ntext=/д('|ʼ|’)артаньян/]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel");
        CHECK_SINGLE_WORD("Squirrel", "squirrel");
        CHECK_SINGLE_WORD("белка", "squirrel");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel");
        CHECK_SINGLE_WORD("Белка", "squirrel");
        CHECK_SINGLE_WORD("456", "number");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼАртаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’Артаньян", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralLogicTextNormReI) {
        TString rules =
            "rule squirrel = ^ (<m> [? ntext=/SqUiRrEl|БеЛкА/i]) $;\n"
            "rule d_artagnan = ^ (<m> [? ntext=/д('|ʼ|’)Артаньян/i]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "squirrel");
        CHECK_SINGLE_WORD("SQUIRREL", "squirrel");
        CHECK_SINGLE_WORD("Squirrel", "squirrel");
        CHECK_SINGLE_WORD("белка", "squirrel");
        CHECK_SINGLE_WORD("БЕЛКА", "squirrel");
        CHECK_SINGLE_WORD("Белка", "squirrel");
        CHECK_SINGLE_WORD("д'Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼАртаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’Артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д'артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("дʼартаньян", "d_artagnan");
        CHECK_SINGLE_WORD("д’артаньян", "d_artagnan");
        CHECK_SINGLE_WORD("Д'АРТАНЬЯН", "d_artagnan");
        CHECK_SINGLE_WORD("ДʼАРТАНЬЯН", "d_artagnan");
        CHECK_SINGLE_WORD("Д’АРТАНЬЯН", "d_artagnan");
    }

    Y_UNIT_TEST(TestLiteralLogicLen) {
        TString rules =
            "rule eq2 = ^ (<m> [? len=2]) $;\n"
            "rule neq2 = ^ (<m> [? len!=2]) $;\n"
            "rule lt2 = ^ (<m> [? len<2]) $;\n"
            "rule gt2 = ^ (<m> [? len>2]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("a", "lt2,neq2");
        CHECK_SINGLE_WORD("ab", "eq2");
        CHECK_SINGLE_WORD("abc", "gt2,neq2");
    }

    Y_UNIT_TEST(TestLiteralLogicLenQuoted) {
        TString rules =
            "rule eq2 = ^ (<m> [? len=\"2\"]) $;\n"
            "rule neq2 = ^ (<m> [? len!=\"2\"]) $;\n"
            "rule lt2 = ^ (<m> [? len<\"2\"]) $;\n"
            "rule gt2 = ^ (<m> [? len>\"2\"]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("a", "lt2,neq2");
        CHECK_SINGLE_WORD("ab", "eq2");
        CHECK_SINGLE_WORD("abc", "gt2,neq2");
    }

    Y_UNIT_TEST(TestLiteralLogicLenZero) {
        TString rules =
            "rule eq0 = ^ (<m> [? len=0]) $;\n"
            "rule neq0 = ^ (<m> [? len!=0]) $;\n"
            "rule lt0 = ^ (<m> [? len<0]) $;\n"
            "rule gt0 = ^ (<m> [? len>0]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("squirrel", "gt0,neq0");
    }

    Y_UNIT_TEST(TestLiteralStringEscaping) {
        TString rules =
            "rule abc = ^ (<m> [\"abc\"]) $;\n"
            "rule dquote = ^ (<m> [\"\\\"\"]) $;\n"
            "rule backslash = ^ (<m> [\"\\\\\"]) $;\n"
            "rule slash = ^ (<m> [\"/\"]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("abc", "abc");
        CHECK_SINGLE_WORD("\"", "dquote");
        CHECK_SINGLE_WORD("\\", "backslash");
        CHECK_SINGLE_WORD("/", "slash");
    }

    Y_UNIT_TEST(TestLiteralReEscaping) {
        TString rules =
            "rule abc = ^ (<m> [? text=/abc/]) $;\n"
            "rule slash = ^ (<m> [? text=/\\//]) $;\n"
            "rule digit = ^ (<m> [? text=/\\d/]) $;\n"
            "rule digit_escaped = ^ (<m> [? text=/\\\\d/]) $;\n"
            "rule dquote = ^ (<m> [? text=/\\\"/]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("abc", "abc");
        CHECK_SINGLE_WORD("/", "slash");
        CHECK_SINGLE_WORD("2", "digit,digit_escaped");
        CHECK_SINGLE_WORD("\"", "dquote");
    }

    Y_UNIT_TEST(TestPropPunct) {
        TString rules =
            "rule punct = ^ (<punct>[? prop=punct]) $;\n"
            ;
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD(".", "punct");
        CHECK_SINGLE_WORD(",", "punct");
        CHECK_SINGLE_WORD(":", "punct");
        CHECK_SINGLE_WORD(";", "punct");

        CHECK_SINGLE_WORD("squirrel", "");
        CHECK_SINGLE_WORD("459", "");
        CHECK_SINGLE_WORD("белка", "");

        CHECK_SINGLE_WORD("meta-squirrel", "");
        CHECK_SINGLE_WORD("д'Артаньян", "");
        CHECK_SINGLE_WORD("д’Артаньян", "");

        CHECK_SINGLE_MULTIWORD(".", "punct");
        CHECK_SINGLE_MULTIWORD(",", "punct");
        CHECK_SINGLE_MULTIWORD(":", "punct");
        CHECK_SINGLE_MULTIWORD(";", "punct");

        CHECK_SINGLE_MULTIWORD("...", "punct");
        CHECK_SINGLE_MULTIWORD("..", "punct");
        CHECK_SINGLE_MULTIWORD(",.", "punct");

        CHECK_SINGLE_MULTIWORD("meta squirrel", "");
        CHECK_SINGLE_MULTIWORD("В.И. Ленин", "");
    }
}

Y_UNIT_TEST_SUITE(RemorphSyntax) {
    Y_UNIT_TEST(LiteralsSimple) {
        CHECK_SYNTAX_VALID("rule r = squirrel;", nullptr);
        CHECK_SYNTAX_VALID("rule r = белка;", nullptr);
        CHECK_SYNTAX_VALID("rule r = 123;", nullptr);
        CHECK_SYNTAX_VALID("rule r = squirrel белка 123;", nullptr);
    }

    Y_UNIT_TEST(LiteralsSimpleQuoted) {
        CHECK_SYNTAX_VALID("rule r = \"squirrel\";", nullptr);
        CHECK_SYNTAX_VALID("rule r = \"белка\";", nullptr);
        CHECK_SYNTAX_VALID("rule r = \"123\";", nullptr);
        CHECK_SYNTAX_VALID("rule r = \"squirrel\" \"белка\" \"123\";", nullptr);
        CHECK_SYNTAX_VALID("rule r = \"squirrel белка 123\";", nullptr);
        CHECK_SYNTAX_VALID("rule r = \"\\\"\";", nullptr);
        CHECK_SYNTAX_VALID("rule r = \"/\";", nullptr);
    }

    Y_UNIT_TEST(LiteralsComplexEscaping) {
        CHECK_SYNTAX_VALID("rule r = [squirrel];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [белка];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [123];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [squirrel белка 123];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [\"squirrel\" \"белка\" \"123\"];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [\"squirrel белка 123\"];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [\"\\\"\"];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [\"\\\\\"];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [\"/\"];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [? text=/\\//];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [? text=/\\d/];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [? text=/\\\\d/];", nullptr);
        CHECK_SYNTAX_VALID("rule r = [? text=/\\\"/];", nullptr);
    }
}

Y_UNIT_TEST_SUITE(RemorphSyntaxIncorrect) {
    Y_UNIT_TEST(LiteralsComparison) {
        CHECK_SYNTAX_INVALID("rule r = [? text>\"squirrel\"];", nullptr);
        CHECK_SYNTAX_INVALID("rule r = [? text<\"squirrel\"];", nullptr);
        CHECK_SYNTAX_INVALID("rule r = [? ntext>\"squirrel\"];", nullptr);
        CHECK_SYNTAX_INVALID("rule r = [? ntext<\"squirrel\"];", nullptr);
    }

    Y_UNIT_TEST(Numbers) {
        CHECK_SYNTAX_INVALID("rule r = [? len=\"squirrel\"];", nullptr);
    }
}

Y_UNIT_TEST_SUITE(RemorphBugfixes) {
    Y_UNIT_TEST(Factex2869) {
        CHECK_SYNTAX_VALID("rule bible_ref = [\"/\" \"(\"] \".\" [?prop!=spbf & text=\":\"];", nullptr);
    }

    Y_UNIT_TEST(Factex2906) {
        TString def = "def any_slash = [\"\\\\\" \"/\"];";
        CHECK_SYNTAX_VALID(def, nullptr);

        TString rules = def + "rule any_slash_rule = $any_slash;";
        TMatcherPtr matcher = TMatcher::Parse(rules);
        TWordSymbols symbols;
        TMatchResults res;

        CHECK_SINGLE_WORD("\\", "any_slash_rule");
        CHECK_SINGLE_WORD("/", "any_slash_rule");
    }

    Y_UNIT_TEST(Factex3100) {
        TTempDir room;
        TFsPath roomPath(room());
        (roomPath / "dir").MkDir();
        TString rulesPath = (roomPath / "rules.rmr").c_str();
        {
            TOFStream rulesStream(rulesPath);
            rulesStream << "\
include \"dir/a.rmi\";\n\
include \"c.rmi\";\n\
";
        }
        {
            TOFStream rulesStream((roomPath / "dir" / "a.rmi").c_str());
            rulesStream << "\
include \"b.rmi\";\n\
";
        }
        {
            TOFStream rulesStream((roomPath / "dir" / "b.rmi").c_str());
        }
        {
            TOFStream rulesStream((roomPath / "c.rmi").c_str());
        }
        NReMorph::NPrivate::TParseResult parseResult;
        {
            TIFStream rulesStream(rulesPath);
            NReMorph::NPrivate::ParseFile(parseResult, rulesPath, rulesStream);
        }
    }
}
