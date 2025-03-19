#include <kernel/remorph/core/compiler.h>
#include <kernel/remorph/core/debug.h>
#include <kernel/remorph/core/executor.h>
#include <kernel/remorph/core/executor_bt.h>
#include <kernel/remorph/core/iterator.h>
#include <kernel/remorph/core/literal.h>
#include <kernel/remorph/core/nfa.h>
#include <kernel/remorph/core/nfa2dfa.h>
#include <kernel/remorph/core/parser.h>
#include <kernel/remorph/core/tokens.h>
#include <kernel/remorph/core/types.h>
#include <kernel/remorph/core/util.h>
#include <kernel/remorph/core/text/text.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/string/vector.h>

using namespace NRemorph;
using namespace NRemorph::NPrivate;

#define UNIT_TEST_LIST                          \
    X(Traversal)\
    X(ParseGoodExpression)\
    X(ParseGoodExpression2)\
    X(ParseGoodExpression3)\
    X(ParseGoodExpression4)\
    X(ParseGoodExpression5)\
    X(ParseGoodExpression6)\
    X(ParseGoodExpression7)\
    X(ParseGoodExpression8)\
    X(ParseGoodExpression9)\
    X(ParseGoodExpression10)\
    X(ParseGoodExpression11)\
    X(ParseGoodExpression12)\
    X(SetNFASS)\
    X(CalcReach)\
    X(CalcReach2)\
    X(CalcEClosure)\
    X(CalcEClosure2)\
    X(Convert)\
    X(Match1)\
    X(Match2)\
    X(Match3)\
    X(Match4)\
    X(Match5)\
    X(Match6)\
    X(Match7)\
    X(Match8)\
    X(Match9)\
    X(Match10)\
    X(Match11)\
    X(Match12)\
    X(Match13)\
    X(Match14)\
    X(Match15)\
    X(Match16)\
    X(Match17)\
    X(Match18)\
    X(Match19)\
    X(Match20)\
    X(Match21)\
    X(Match22)\
    X(Match23)\
    X(Match24)\
    X(Match25)\
    X(Match26)\
    X(Match27)\
    X(Match28)\
    X(Match29)\
    X(Match30)\
    X(CombineNFAs)\
    X(CombineNFAs2)\
    X(CombineNFAs3)\
    X(PreParsedTokens)\
    X(MatchAll1)\
    X(MatchAll2)\
    X(SearchAll)

#define UNIT_TEST_EXCEPTION_LIST\
    X(ParseBadParen1, NRemorph::TParserError)\
    X(ParseBadParen2, NRemorph::TParserError)\
    X(ParseUnexpectedToken1, NRemorph::TParserError)\
    X(ParseUnexpectedToken2, NRemorph::TParserError)\
    X(ParseUnexpectedToken3, NRemorph::TParserError)\
    X(ParseUnexpectedToken4, NRemorph::TParserError)\
    X(ParseUnexpectedToken5, NRemorph::TParserError) \
    X(ParseBadRepeater1, NRemorph::TParserError)\
    X(ParseBadRepeater2, NRemorph::TParserError)\
    X(ParseBadRepeater3, NRemorph::TParserError) \
    X(ParseBadRepeater4, NRemorph::TParserError) \
    X(ParseBadRepeater5, NRemorph::TParserError)

inline bool operator==(const TMatchInfo& l, const TMatchInfo& r) {
    if (l.Submatches != r.Submatches)
        return false;
    if (l.NamedSubmatches.size() != r.NamedSubmatches.size())
        return false;
    TVector<std::pair<TString, TSubmatch>> lNames(l.NamedSubmatches.begin(), l.NamedSubmatches.end());
    ::StableSort(lNames.begin(), lNames.end());
    TVector<std::pair<TString, TSubmatch>> rNames(r.NamedSubmatches.begin(), r.NamedSubmatches.end());
    ::StableSort(rNames.begin(), rNames.end());
    return lNames == rNames;
}

TVector<TSubmatch> GetNamedSubmatches(const TNamedSubmatches& subs, const TString& name) {
    TVector<TSubmatch> res;
    std::pair<TNamedSubmatches::const_iterator, TNamedSubmatches::const_iterator> range = subs.equal_range(name);
    for (TNamedSubmatches::const_iterator i = range.first; i != range.second; ++i) {
        res.push_back(i->second);
    }
    ::StableSort(res.begin(), res.end());
    return res;
}

class RemorphCoreRe: public TTestBase {
private:
    UNIT_TEST_SUITE(RemorphCoreRe);
#define X(A, B) UNIT_TEST_EXCEPTION(A, B)
    UNIT_TEST_EXCEPTION_LIST
#undef X
#define X(A) UNIT_TEST(A);
    UNIT_TEST_LIST
#undef X
    UNIT_TEST_SUITE_END();

private:
    struct TData {
        static const size_t NumTags = 2;
        TTagIdToIndex ti_empty;
        TTagIdToIndex ti0_0;
        TTagIdToIndex ti1_0;
        TTagIdToIndex ti0_0__1_0;
        TTagIdToIndex ti0_1__1_0;
        TData() {
            ti0_0[0] = 0;
            ti1_0[1] = 0;

            ti0_0__1_0[0] = 0;
            ti0_0__1_0[1] = 0;

            ti0_1__1_0[0] = 1;
            ti0_1__1_0[1] = 0;
        }
    };
    typedef THolder<TData> TDataPtr;

private:
    TDataPtr Data;

private:
    void SetUp() override {
        Data.Reset(new TData());
    }

    void TearDown() override {
        Data.Reset(nullptr);
    }

    static TMatchInfoPtr Match(const TString& re, const TString& input) {
        return Match(re, input, TString());
    }

    static TMatchInfoPtr Match(const TString& re, const TString& input, const TString& dotFileName) {
        TVectorTokens tokens;
        TLiteralTableWtroka lt;
        Parse(lt, tokens, UTF8ToWide(re));
        TAstNodePtr ast = NRemorph::Parse(lt, tokens);
        TNFAPtr nfa = NRemorph::CompileNFA(lt, ast.Get());
        if (!!dotFileName) {
            TUnbufferedFileOutput out(dotFileName + "_nfa.dot");
            PrintAsDot(out, lt, *nfa);
        }
        TDFAPtr dfa = NRemorph::Convert(lt, *nfa);
        if (!!dotFileName) {
            TUnbufferedFileOutput out(dotFileName + "_dfa.dot");
            PrintAsDot(out, lt, *dfa);
        }
        TVectorSymbols inputV;
        Parse(UTF8ToWide(input), inputV);
        //TMatchInfoPtr resultNFA = NRemorph::Match(lt, NRemorph::TVectorIterator<TUtf16String>(inputV), *nfa);
        TMatchInfoPtr resultDFA = MatchText(lt, *dfa, NRemorph::TVectorIterator<TUtf16String>(inputV));
        //bool bResultNFA = resultNFA.Get();
        bool bResultDFA = resultDFA.Get();
        //Cdbg << "NFA: " << (bResultNFA ? "matched" : "not-matched") << ",";
        //if (bResultNFA)
        //    Cdbg << *resultNFA << Endl;
        Cdbg << "DFA: " << (bResultDFA ? "matched" : "not-matched") << ",";
        if (bResultDFA)
            Cdbg << *resultDFA << Endl;
        //UNIT_ASSERT_EQUAL(bResultNFA, bResultDFA);
        //UNIT_ASSERT(bResultNFA);
        UNIT_ASSERT(bResultDFA);
        //UNIT_ASSERT_EQUAL(*resultNFA, *resultDFA);
        return resultDFA;
    }

    static TMatchInfoPtr MatchCombined(const TVector<TString>& v, const TString& input) {
        return MatchCombined(v, input, TString());
    }

    static TMatchInfoPtr MatchCombined(const TVector<TString>& v, const TString& input, const TString& dotFileName) {
        TVectorNFAs nfas;
        TLiteralTableWtroka lt;
        for (size_t i = 0; i < v.size(); ++i) {
            TVectorTokens tokens;
            Parse(lt, tokens, UTF8ToWide(v[i]));
            nfas.push_back(NRemorph::CompileNFA(lt, tokens));
        }
        TNFAPtr nfa = NRemorph::Combine(nfas);
        if (!!dotFileName) {
            TUnbufferedFileOutput out(dotFileName + "_nfa.dot");
            PrintAsDot(out, lt, *nfa);
        }
        TDFAPtr dfa = NRemorph::Convert(lt, *nfa);
        if (!!dotFileName) {
            TUnbufferedFileOutput out(dotFileName + "_dfa.dot");
            PrintAsDot(out, lt, *dfa);
        }
        TVectorSymbols inputV;
        Parse(UTF8ToWide(input), inputV);
        TMatchInfoPtr resultNFA = NRemorph::Match(lt, NRemorph::TVectorIterator<TUtf16String>(inputV), *nfa);
        TMatchInfoPtr resultDFA = MatchText(lt, *dfa, NRemorph::TVectorIterator<TUtf16String>(inputV));
        bool bResultNFA = resultNFA.Get();
        bool bResultDFA = resultDFA.Get();
        Cdbg << "NFA: " << (bResultNFA ? "matched" : "not-matched") << ",";
        if (bResultNFA)
            Cdbg << *resultNFA << Endl;
        Cdbg << "DFA: " << (bResultDFA ? "matched" : "not-matched") << ",";
        if (bResultDFA)
            Cdbg << *resultDFA << Endl;
        UNIT_ASSERT_EQUAL(bResultNFA, bResultDFA);
        UNIT_ASSERT(bResultNFA);
        UNIT_ASSERT(bResultDFA);
        UNIT_ASSERT_EQUAL(*resultNFA, *resultDFA);
        return resultDFA;
    }

    static void MatchAllCombined(const TVector<TString>& v, const TString& input, const TString& dotFileName, TVector<TMatchInfoPtr>& matches) {
        TVectorNFAs nfas;
        TLiteralTableAscii lt;
        for (size_t i = 0; i < v.size(); ++i) {
            TVectorTokens tokens;
            Parse(tokens, v[i]);
            nfas.push_back(NRemorph::CompileNFA(lt, tokens));
        }
        TNFAPtr nfa = NRemorph::Combine(nfas);
        if (!!dotFileName) {
            TUnbufferedFileOutput out(dotFileName + "_nfa.dot");
            PrintAsDot(out, lt, *nfa);
        }
        TDFAPtr dfa = NRemorph::Convert(lt, *nfa);
        if (!!dotFileName) {
            TUnbufferedFileOutput out(dotFileName + "_dfa.dot");
            PrintAsDot(out, lt, *dfa);
        }
        TVector<char> inputV(input.size());
        memcpy(&inputV[0], input.data(), input.size());
        MatchAllText(lt, *dfa, NRemorph::TVectorIterator<char>(inputV), matches);
    }



public:
#define X(A, B) void A();
    UNIT_TEST_EXCEPTION_LIST
#undef X
#define X(A) void A();
    UNIT_TEST_LIST
#undef X
};

UNIT_TEST_SUITE_REGISTRATION(RemorphCoreRe);

void RemorphCoreRe::ParseGoodExpression() {
    TString s = "( мама * (?<bar> (?: мыла ? ) + раму + ) ? ) *";
    TString canon =
        "LParen Literal[мама] Asterisk LParen[bar] LParen-NC Literal[мыла] Question RParen Plus Literal[раму] Plus RParen Question RParen Asterisk\n"
        "Iteration{0,-1}\n"
        "  Submatch[0]\n"
        "    Catenation\n"
        "      Iteration{0,-1}\n"
        "        Literal[мама]\n"
        "      Iteration{0,1}\n"
        "        Submatch[0-bar]\n"
        "          Catenation\n"
        "            Iteration{1,-1}\n"
        "              Iteration{0,1}\n"
        "                Literal[мыла]\n"
        "            Iteration{1,-1}\n"
        "              Literal[раму]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression2() {
    TString s = "a ( b ) | ( c )";
    TString canon =
        "Literal[a] LParen Literal[b] RParen Pipe LParen Literal[c] RParen\n"
        "Union\n"
        "  Catenation\n"
        "    Literal[a]\n"
        "    Submatch[0]\n"
        "      Literal[b]\n"
        "  Submatch[0]\n"
        "    Literal[c]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression3() {
    TString s = "^ $";
    TString canon =
        "Literal[BOS] Literal[EOS]\n"
        "Catenation\n"
        "  Literal[BOS]\n"
        "  Literal[EOS]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression4() {
    TString s = ".";
    TString canon =
        "Literal[ANY]\n"
        "Literal[ANY]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression5() {
    TString s = ". +";
    TString canon =
        "Literal[ANY] Plus\n"
        "Iteration{1,-1}\n"
        "  Literal[ANY]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression6() {
    TString s = ". {2,3}";
    TString canon =
        "Literal[ANY] Repeat{2,3}\n"
        "Iteration{2,3}\n"
        "  Literal[ANY]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression7() {
    TString s = ". {0,3}";
    TString canon =
        "Literal[ANY] Repeat{0,3}\n"
        "Iteration{0,3}\n"
        "  Literal[ANY]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression8() {
    TString s = ". {2,-1}";
    TString canon =
        "Literal[ANY] Repeat{2,-1}\n"
        "Iteration{2,-1}\n"
        "  Literal[ANY]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression9() {
    TString s = ". {1,1}";
    TString canon =
        "Literal[ANY] Repeat{1,1}\n"
        "Iteration{1,1}\n"
        "  Literal[ANY]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression10() {
    TString s = ". {0,1}";
    TString canon =
        "Literal[ANY] Repeat{0,1}\n"
        "Iteration{0,1}\n"
        "  Literal[ANY]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression11() {
    TString s = ". {0,-1}";
    TString canon =
        "Literal[ANY] Repeat{0,-1}\n"
        "Iteration{0,-1}\n"
        "  Literal[ANY]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseGoodExpression12() {
    TString s = ". {1,-1}";
    TString canon =
        "Literal[ANY] Repeat{1,-1}\n"
        "Iteration{1,-1}\n"
        "  Literal[ANY]\n"
        ;
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));

    TString outS;
    TStringOutput out(outS);
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    out << Bind(lt, tokens) << Endl << Bind(lt, *ast);
    Cdbg << outS << Endl << canon << Endl;
    UNIT_ASSERT_EQUAL(outS, canon);
}

void RemorphCoreRe::ParseBadParen1() {
    TString s = "( мама ( мыла )";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseBadParen2() {
    TString s = "( мама ) мыла )";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseUnexpectedToken1() {
    TString s = "? ( мама ( мыла ) )";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseUnexpectedToken2() {
    TString s = "( мама ( мыла ) ) ? *";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseUnexpectedToken3() {
    TString s = "( + )";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseUnexpectedToken4() {
    TString s = "^ +";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseUnexpectedToken5() {
    TString s = "$ +";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseBadRepeater1() {
    TString s = ". {-1,3}";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseBadRepeater2() {
    TString s = ". {-2,3}";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseBadRepeater3() {
    TString s = ". {0,0}";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseBadRepeater4() {
    TString s = ". {2,1}";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::ParseBadRepeater5() {
    TString s = ". {2,-2}";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
}

void RemorphCoreRe::Match1() {
    // TDBGOFlagsGuard g(
    //     // DO_EXECUTOR | DO_CONVERTER_ADD |
    //     DO_CONVERTER_FIND |
    //     DO_CONVERTER_CONVERT |
    //     DO_CONVERTER_NI
    //     );
    TMatchInfoPtr result = Match(
        "a ( b ? ) c"
        , "a b c"
        // , "TestMatch1"
        );
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);
}

void RemorphCoreRe::Match2() {
    TMatchInfoPtr result = Match("мама ( мыла ? ) раму", "мама раму");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, size_t(-1));
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, size_t(-1));
}

void RemorphCoreRe::Match3() {
    TMatchInfoPtr result = Match("мама ( мыла ? ) (?: раму )", "мама мыла раму");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);
}

void RemorphCoreRe::Match4() {
    // TDBGOFlagsGuard g(
    //     // DO_EXECUTOR | DO_CONVERTER_ADD |
    //     DO_CONVERTER_FIND |
    //     DO_CONVERTER_CONVERT);
    TMatchInfoPtr result = Match(
        "a ( b ) + c"
        , "a b b c"
        // , "TestMatch4"
        );
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 3);
}

void RemorphCoreRe::Match5() {
    TMatchInfoPtr result = Match(
        "( a * ) a * a",
        "a a a a a"
        );
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 4);
}

void RemorphCoreRe::Match6() {
    TMatchInfoPtr result = Match("( a + ) a ? a", "a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);
}

void RemorphCoreRe::Match7() {
    // TDBGOFlagsGuard g(
    //     // DO_EXECUTOR |
    //     // DO_CONVERTER_ADD |
    //     // DO_CONVERTER_FIND |
    //     DO_CONVERTER_CONVERT);
    TMatchInfoPtr result = Match(
        "( ( a * ) ( a ? a ? ) )"
        , "a a a a a"
        // , "TestMatch7"
        );
    UNIT_ASSERT(result);

    UNIT_ASSERT_EQUAL(result->Submatches.size(), 3);

    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 5);

    UNIT_ASSERT_EQUAL(result->Submatches[1].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[1].second, 5);

    UNIT_ASSERT_EQUAL(result->Submatches[2].first, size_t(-1));
    UNIT_ASSERT_EQUAL(result->Submatches[2].second, size_t(-1));
}

void RemorphCoreRe::Match16() {
    TMatchInfoPtr result = Match("a * ( a ? a ? )", "a a a a a");
    UNIT_ASSERT(result);

    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);

    UNIT_ASSERT_EQUAL(result->Submatches[0].first, size_t(-1));
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, size_t(-1));
}

void RemorphCoreRe::Match8() {
    TMatchInfoPtr result = Match("a ( b ) + a", "a b b b a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 3);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 4);
}

void RemorphCoreRe::Match9() {
    TMatchInfoPtr result = Match(
        "( . + ) ( a )"
        , "b b a"
        // , "TestMatch9"
        );
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);
}

void RemorphCoreRe::Match10() {
    TMatchInfoPtr result = Match(
        "( . + ) a b"
        ,
        "c d a b");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);
}

void RemorphCoreRe::Match11() {
    TMatchInfoPtr result = Match("мама (?<foo> мыла ? ) (?<bar> раму )", "мама мыла раму");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.count("foo"), 1);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("foo")->second.first, 1);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("foo")->second.second, 2);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.count("bar"), 1);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("bar")->second.first, 2);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("bar")->second.second, 3);
}

void RemorphCoreRe::Match12() {
    TMatchInfoPtr result = Match("мама (?<foo> мыла ? ) (?<foo> раму )", "мама мыла раму");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.count("foo"), 2);
    TVector<TSubmatch> etalon;
    etalon.push_back(TSubmatch(1, 2));
    etalon.push_back(TSubmatch(2, 3));
    UNIT_ASSERT_EQUAL(GetNamedSubmatches(result->NamedSubmatches, "foo"), etalon);
}

void RemorphCoreRe::Match13() {
    TMatchInfoPtr result = Match(
        "как (?: переводится | перевести ) (?: на русский (?<source> . ) | (?<source> . ) на русский )",
        "как переводится на русский table");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.count("source"), 1);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("source")->second.first, 4);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("source")->second.second, 5);
}

void RemorphCoreRe::Match14() {
    TMatchInfoPtr result = Match(
        "(?: (?: слово (?<text> . + ) ) | (?<text> . + ) ) на (?<lang> русский ) перевод ?"
        ,
        "table на русский"
        );
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.size(), 2);

    UNIT_ASSERT_EQUAL(result->NamedSubmatches.count("text"), 1);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("text")->second.first, 0);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("text")->second.second, 1);

    UNIT_ASSERT_EQUAL(result->NamedSubmatches.count("lang"), 1);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("lang")->second.first, 2);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("lang")->second.second, 3);
}

void RemorphCoreRe::Match15() {
    TMatchInfoPtr result = Match(
        "как (?: слово (?<text> . + ) | (?<text> . + ) ) на (?<lang> русский | английский )"
        ,
        "как слово мышь на английский"
        );
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.size(), 2);

    UNIT_ASSERT_EQUAL(result->NamedSubmatches.count("text"), 1);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("text")->second.first, 2);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("text")->second.second, 3);

    UNIT_ASSERT_EQUAL(result->NamedSubmatches.count("lang"), 1);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("lang")->second.first, 4);
    UNIT_ASSERT_EQUAL(result->NamedSubmatches.find("lang")->second.second, 5);
}

void RemorphCoreRe::Match17() {
    TMatchInfoPtr result = Match(
        "( a ? ) a ( a ? )"
        ,
        "a a"
        );
    UNIT_ASSERT(result);

    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);

    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 1);

    UNIT_ASSERT_EQUAL(result->Submatches[1].first, size_t(-1));
    UNIT_ASSERT_EQUAL(result->Submatches[1].second, size_t(-1));
}

void RemorphCoreRe::Match18() {
    TMatchInfoPtr result = Match(
        "( a ) (?: . + ) ( b )"
        ,
        "a x y b"
        );
    UNIT_ASSERT(result);

    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);

    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 1);

    UNIT_ASSERT_EQUAL(result->Submatches[1].first, 3);
    UNIT_ASSERT_EQUAL(result->Submatches[1].second, 4);
}

void RemorphCoreRe::Match19() {
    // TDBGOFlagsGuard g(DO_CONVERTER_ADD | DO_CONVERTER_FIND | DO_CONVERTER_CONVERT);
    TMatchInfoPtr result = Match(
        "b c | ( . + ) ( c )"
        , "b x c"
        // , "TestMatch19"
        );
    UNIT_ASSERT(result);

    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);

    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);

    UNIT_ASSERT_EQUAL(result->Submatches[1].first, 2);
    UNIT_ASSERT_EQUAL(result->Submatches[1].second, 3);
}

void RemorphCoreRe::Match20() {
    // TDBGOFlagsGuard g(DO_CONVERTER_ADD | DO_CONVERTER_FIND | DO_CONVERTER_CONVERT);
    TMatchInfoPtr result = Match(
        "b (?: c (?<foo> . ) | (?<foo> . ) )"
        , "b c x"
        // , "TestMatch20"
        );
    UNIT_ASSERT(result);

    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);

    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 3);

    UNIT_ASSERT_EQUAL(result->Submatches[1].first, size_t(-1));
    UNIT_ASSERT_EQUAL(result->Submatches[1].second, size_t(-1));
}

void RemorphCoreRe::Match21() {
    // TDBGOFlagsGuard g(
    //     DO_MATCH
    //     | DO_CONVERTER_ADD
    //     | DO_CONVERTER_FIND
    //     | DO_CONVERTER_CONVERT
    //     );
    TMatchInfoPtr result = Match(
        "(?: (?: a | b ) + ) ( a (?: a | b ) | b (?: a | b ) )"
        , "a a a"
        // , "TestMatch21"
        );
    UNIT_ASSERT(result);

    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);

    UNIT_ASSERT_EQUAL(result->Submatches[0].second - result->Submatches[0].first, 2);
}

void RemorphCoreRe::Match22() {
    // TDBGOFlagsGuard g(
    //     DO_MATCH
    //     | DO_CONVERTER_ADD
    //     | DO_CONVERTER_FIND
    //     | DO_CONVERTER_CONVERT
    //     );
    TMatchInfoPtr result = Match(
        "(?: . + ) ( a . | b . )"
        , "a a a"
        // , "TestMatch22"
        );
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second - result->Submatches[0].first, 2);

    result = Match(
        "(?: . + ) ( a . | b . )"
        , "c a c"
        // , "TestMatch22"
        );
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second - result->Submatches[0].first, 2);

    result = Match(
        "(?: . + ) ( a . | b . )"
        , "b b b"
        // , "TestMatch22"
        );
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second - result->Submatches[0].first, 2);
}

void RemorphCoreRe::Match23() {
    TMatchInfoPtr result = Match(
        "( . {2,4} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 4);

    result = Match(
        "( . {2,4} ) ( . * )",
        "a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);

    result = Match(
        "( . {2,4} ) ( . * )",
        "a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 3);

    result = Match(
        "( . {2,4} ) ( . * )",
        "a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 4);
}

void RemorphCoreRe::Match24() {
    TMatchInfoPtr result = Match(
        "( . {1} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 1);

    result = Match(
        "( . {1,1} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 1);

    result = Match(
        "( . {1,1} ) ( . * )",
        "a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 1);
}

void RemorphCoreRe::Match25() {
    TMatchInfoPtr result = Match(
        "( . {0,1} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 1);

    result = Match(
        "( a {0,1} ) ( b * )",
        "b");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, size_t(-1));
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, size_t(-1));
}

void RemorphCoreRe::Match26() {
    TMatchInfoPtr result = Match(
        "( . {1,} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 5);

    result = Match(
        "( . {1,-1} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 5);

    result = Match(
        "( . {1,} ) ( . * )",
        "a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 1);
}

void RemorphCoreRe::Match27() {
    TMatchInfoPtr result = Match(
        "( . {0,} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 5);

    result = Match(
        "( . {0,-1} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 5);

    result = Match(
        "( a {0,} ) ( b * )",
        "b");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, size_t(-1));
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, size_t(-1));
}

void RemorphCoreRe::Match28() {
    TMatchInfoPtr result = Match(
        "( . {0,3} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 3);

    result = Match(
        "( a {0,3} ) ( b * )",
        "b");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, size_t(-1));
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, size_t(-1));
}

void RemorphCoreRe::Match29() {
    TMatchInfoPtr result = Match(
        "( . {2,} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 5);

    result = Match(
        "( . {2,-1} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 5);

    result = Match(
        "( . {2,} ) ( . * )",
        "a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);
}

void RemorphCoreRe::Match30() {
    TMatchInfoPtr result = Match(
        "( . {2} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);

    result = Match(
        "( . {2,2} ) ( . * )",
        "a a a a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);

    result = Match(
        "( . {2,2} ) ( . * )",
        "a a");
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);
}

//void RemorphCoreRe::Match() {
//    TMatchInfoPtr result = Match(
//        "( . * ) . *",
//        "a a a a a");
//    UNIT_ASSERT(result);
//    UNIT_ASSERT_EQUAL(result->Submatches.size(), 1);
//    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 0);
//    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 5);
//}

struct TNodeData: public TSimpleRefCount<TNodeData> {
    TString Value;
    TNodeData(const TString& s)
        : Value(s) {
    }
};

typedef NRemorph::TTree<TNodeData> TTestTree;
typedef TTestTree::TNode TNode;
typedef TTestTree::TNodePtr TNodePtr;

struct TVisitor {
    IOutputStream& Out;
    bool Before;
    bool After;

    TVisitor(IOutputStream& out, bool before = true, bool after = true)
        : Out(out)
        , Before(before)
        , After(after)
    {
    }
    bool VisitBeforeChildren(TNode* node) {
        if (Before)
            Out << node->Data->Value;
        return true;
    }
    bool VisitAfterChildren(TNode* node) {
        if (After)
            Out << node->Data->Value;
        return true;
    }
};

void RemorphCoreRe::Traversal() {
    // static const char* tree =
    //     "a"
    //     " b"
    //     "  d"
    //     " c"
    //     "  e"
    //     "  f";

    TNodePtr a(new TNode(new TNodeData("a")));
    TNodePtr b(new TNode(new TNodeData("b")));
    TNodePtr c(new TNode(new TNodeData("c")));
    TNodePtr d(new TNode(new TNodeData("d")));
    TNodePtr e(new TNode(new TNodeData("e")));
    TNodePtr f(new TNode(new TNodeData("f")));

    a->Left = b.Get();
    a->Right = c.Get();

    b->Left = d.Get();
    b->Parent = a.Get();

    c->Left = e.Get();
    c->Right = f.Get();
    c->Parent = a.Get();

    d->Parent = b.Get();

    e->Parent = c.Get();
    f->Parent = c.Get();

    TString outS;
    TStringOutput out(outS);
    TVisitor v(out);

    TTestTree::Traverse(a.Get(), v);
    UNIT_ASSERT_EQUAL(outS, "abddbceeffca");

    outS.clear();
    TTestTree::Traverse(c.Get(), v);
    Cdbg << outS << Endl;
    UNIT_ASSERT_EQUAL(outS, "ceeffc");

    outS.clear();
    TVisitor vBefore(out, true, false);
    TTestTree::Traverse(a.Get(), vBefore);
    Cdbg << outS << Endl;
    UNIT_ASSERT_EQUAL(outS, "abdcef");

    outS.clear();
    TVisitor vAfter(out, false, true);
    TTestTree::Traverse(a.Get(), vAfter);
    Cdbg << outS << Endl;
    UNIT_ASSERT_EQUAL(outS, "dbefca");
}

void RemorphCoreRe::PreParsedTokens() {
    TUtf16String w1 = u"мама ( мыла ? )";
    TUtf16String w2 = u"( раму с )";
    TUtf16String w3 = u"мылом";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, w2);
    Cdbg << Bind(lt, tokens) << Endl;

    TAstNodePtr var = NRemorph::Parse(lt, tokens);

    tokens.clear();
    Parse(lt, tokens, w1);
    Cdbg << Bind(lt, tokens) << Endl;

    tokens.push_back(new NRemorph::TTokenRE(var.Get()));
    Cdbg << Bind(lt, tokens) << Endl;

    Parse(lt, tokens, w3);
    Cdbg << Bind(lt, tokens) << Endl;

    TAstNodePtr ast = NRemorph::Parse(lt, tokens);

    TNFAPtr nfa = NRemorph::CompileNFA(lt, ast.Get());
    TString inputS = "мама мыла раму с мылом";
    TVectorSymbols input;
    Parse(UTF8ToWide(inputS), input);
    TMatchInfoPtr result = NRemorph::Match(lt, NRemorph::TVectorIterator<TUtf16String>(input), *nfa);
    UNIT_ASSERT(result);
    UNIT_ASSERT_EQUAL(result->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(result->Submatches[0].first, 1);
    UNIT_ASSERT_EQUAL(result->Submatches[0].second, 2);
    UNIT_ASSERT_EQUAL(result->Submatches[1].first, 2);
    UNIT_ASSERT_EQUAL(result->Submatches[1].second, 4);
}

void RemorphCoreRe::CalcReach() {
    TString s = "( (?: a | b ) * ) a";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    TNFAPtr nfa = NRemorph::CompileNFA(lt, ast.Get());

    TLiteral a(lt.Get(u"a"));
    TLiteral b(lt.Get(u"b"));
    TNFAStateSetPtr source(new TNFAStateSet(*nfa));
    TNFAStateSetPtr canonic(new TNFAStateSet(*nfa));
    source->Insert(2);
    canonic->Insert(5, 4);
    TNFAStateSetPtr result = source->CalcReach(a);
    Cdbg << *result << Endl;
    Cdbg << *canonic << Endl;
    UNIT_ASSERT_EQUAL(*result, *canonic);

    source = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    source->Insert(0);
    source->Insert(1);
    source->Insert(2);
    source->Insert(3);
    source->Insert(4);
    source->Insert(5);
    source->Insert(6);
    source->Insert(7);
    source->Insert(8);
    canonic = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    canonic->Insert(5, 4);
    canonic->Insert(8, 8);
    result = source->CalcReach(a);
    Cdbg << *result << Endl;
    Cdbg << *canonic << Endl;
    UNIT_ASSERT_EQUAL(*result, *canonic);

    source = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    source->Insert(0);
    source->Insert(1);
    source->Insert(2);
    source->Insert(3);
    source->Insert(4);
    source->Insert(5);
    source->Insert(6);
    source->Insert(7);
    source->Insert(8);
    canonic = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    canonic->Insert(5, 10);
    result = source->CalcReach(b);
    Cdbg << *result << Endl;
    Cdbg << *canonic << Endl;
    UNIT_ASSERT_EQUAL(*result, *canonic);
}

void RemorphCoreRe::CalcReach2() {
    TString s = "a ( b ) + a";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    TNFAPtr nfa = NRemorph::CompileNFA(lt, ast.Get());

    TLiteral a(lt.Get(u"a"));
    TLiteral b(lt.Get(u"b"));
    TNFAStateSetPtr source(TNFAStateSetPtr(new TNFAStateSet(*nfa)));
    TNFAStateSetPtr canonic(TNFAStateSetPtr(new TNFAStateSet(*nfa)));

    source->Insert(5);
    source->Insert(0);

    canonic->Insert(2, 1);
    canonic->Insert(6, 7);

    TNFAStateSetPtr result = source->CalcReach(a);
    Cdbg << *result << Endl;
    UNIT_ASSERT_EQUAL(*result, *canonic);

    source = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    source->Insert(1);
    source->Insert(0);

    canonic = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    canonic->Insert(3, 3);

    result = source->CalcReach(b);
    Cdbg << *result << Endl;
    UNIT_ASSERT_EQUAL(*result, *canonic);

    source = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    source->Insert(0);
    source->Insert(1);
    source->Insert(2);
    source->Insert(3);
    source->Insert(4);

    canonic = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    canonic->Insert(3, 3);

    result = source->CalcReach(b);
    Cdbg << *result << Endl;
    UNIT_ASSERT_EQUAL(*result, *canonic);
}

void RemorphCoreRe::Convert() {
    TString s = "a ( b ) + a";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    TNFAPtr nfa = NRemorph::CompileNFA(lt, ast.Get());
    NRemorph::Convert(lt, *nfa);
}

void RemorphCoreRe::CalcEClosure() {
    TString s = "( (?: a | b ) * ) a";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    Cdbg << Bind(lt, tokens) << Endl;

    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    TNFAPtr nfa = NRemorph::CompileNFA(lt, ast.Get());

    TNFAStateSetPtr source(TNFAStateSetPtr(new TNFAStateSet(*nfa)));
    TNFAStateSetPtr canonic(TNFAStateSetPtr(new TNFAStateSet(*nfa)));

    source->Insert(nfa->Start->Id);
    canonic->Insert(nfa->Start->Id);
    canonic->Insert(0, 7, Data->ti0_0__1_0);
    canonic->Insert(1, 9, Data->ti0_0);
    canonic->Insert(2, 3, Data->ti0_0);
    canonic->Insert(3, 2, Data->ti0_0);
    canonic->Insert(4, 1, Data->ti0_0);
    canonic->Insert(7, 11, Data->ti0_0);

    TTagIdToIndex newIndexes;
    TNFAStateSet closure1(*source);
    closure1.CalcEClosure(newIndexes);
    Cdbg << *source << Endl;
    Cdbg << closure1 << Endl;
    Cdbg << *canonic << Endl;
    UNIT_ASSERT_EQUAL(closure1, *canonic);

    source = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    source->Insert(4);

    canonic = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    canonic->Insert(0, 7, Data->ti1_0);
    canonic->Insert(1, 9);
    canonic->Insert(2, 3);
    canonic->Insert(3, 2);
    canonic->Insert(4);
    canonic->Insert(7, 11);

    TNFAStateSet closure2(*source);
    closure2.CalcEClosure(newIndexes);
    Cdbg << *source << Endl;
    Cdbg << closure2 << Endl;
    Cdbg << *canonic << Endl;
    UNIT_ASSERT_EQUAL(closure2, *canonic);

    source = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    source->Insert(0);
    source->Insert(3);
    canonic = TNFAStateSetPtr(new TNFAStateSet(*nfa));
    canonic->Insert(0);
    canonic->Insert(1, 9);
    canonic->Insert(2, 3);
    canonic->Insert(3);

    TNFAStateSet closure3(*source);
    closure3.CalcEClosure(newIndexes);
    Cdbg << *source << Endl;
    Cdbg << closure3 << Endl;
    Cdbg << *canonic << Endl;
    UNIT_ASSERT_EQUAL(closure3, *canonic);
}

void RemorphCoreRe::CalcEClosure2() {
    TString s = "a ( b ) + a";
    TVectorTokens tokens;
    TLiteralTableWtroka lt;
    Parse(lt, tokens, UTF8ToWide(s));
    Cdbg << Bind(lt, tokens) << Endl;

    TAstNodePtr ast = NRemorph::Parse(lt, tokens);
    TNFAPtr nfa = NRemorph::CompileNFA(lt, ast.Get());

    TNFAStateSet source(*nfa);
    TNFAStateSet canonic(*nfa);

    source.Insert(3, Data->ti0_0);

    canonic.Insert(3, Data->ti0_0);
    canonic.Insert(4, 4, Data->ti0_0__1_0);
    canonic.Insert(0, 6, Data->ti0_0__1_0);
    canonic.Insert(2, 5, Data->ti0_0__1_0);
    canonic.Insert(1, 2, Data->ti0_1__1_0);

    TTagIdToIndex newIndexes;
    TNFAStateSet closure(source);
    closure.CalcEClosure(newIndexes);
    Cdbg << closure << Endl;
    Cdbg << canonic << Endl;
    UNIT_ASSERT_EQUAL(closure, canonic);
}

void RemorphCoreRe::SetNFASS() {
    TSetNFASS setNFASS;

    TNFA nfa;
    nfa.NumSubmatches = 2;
    nfa.CreateState();
    nfa.CreateState();

    TNFAStateSet s1(nfa);
    s1.Insert(0, Data->ti0_0__1_0);
    s1.Insert(1, Data->ti0_0);
    Cdbg << s1 << Endl;

    TNFAStateSet s2(nfa);
    s2.Insert(0, Data->ti0_0__1_0);
    s2.Insert(1, Data->ti0_0__1_0);
    Cdbg << s2 << Endl;

    UNIT_ASSERT(!(s1 == s2));

    setNFASS.insert(&s1);
    UNIT_ASSERT_EQUAL(&s2, *setNFASS.insert(&s2).first);
}

void RemorphCoreRe::CombineNFAs() {
    TVector<TString> v;
    v.push_back("( a ) b ( c )");
    v.push_back("a ( b ) c");

    TMatchInfoPtr r = MatchCombined(v, "a b c");

    UNIT_ASSERT_EQUAL(r->Submatches.size(), 2);

    UNIT_ASSERT_EQUAL(r->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(r->Submatches[0].second, 1);

    UNIT_ASSERT_EQUAL(r->Submatches[1].first, 2);
    UNIT_ASSERT_EQUAL(r->Submatches[1].second, 3);
}

void RemorphCoreRe::CombineNFAs2() {
    TVector<TString> v;
    v.push_back("a b (?<foo> c )");

    v.push_back("(?<bar> a ) ( . + ) (?<baz> c )");
    v.push_back("(?<boz> a ) b c");

    TMatchInfoPtr r = MatchCombined(
        v
        , "a b b c"
        // , "TestCombineNFAs2"
        );

    UNIT_ASSERT_EQUAL(r->Submatches.size(), 3);
    UNIT_ASSERT_EQUAL(r->MatchedId, 1);

    UNIT_ASSERT_EQUAL(r->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(r->Submatches[0].second, 1);

    UNIT_ASSERT_EQUAL(r->Submatches[1].first, 1);
    UNIT_ASSERT_EQUAL(r->Submatches[1].second, 3);

    UNIT_ASSERT_EQUAL(r->Submatches[2].first, 3);
    UNIT_ASSERT_EQUAL(r->Submatches[2].second, 4);

    r = MatchCombined(v, "a b c");

    UNIT_ASSERT_EQUAL(r->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(r->MatchedId, 0);

    UNIT_ASSERT_EQUAL(r->Submatches[0].first, 2);
    UNIT_ASSERT_EQUAL(r->Submatches[0].second, 3);
}

void RemorphCoreRe::CombineNFAs3() {
    TVector<TString> v;
    v.push_back("a b (?<foo> c )");
    v.push_back("(?<bar> a ) ( . * ) (?<baz> c )");
    v.push_back("(?<boz> a ) b c");

    TMatchInfoPtr r = MatchCombined(
        v
        , "a b b c"
        // , "TestCombineNFAs3"
        );

    UNIT_ASSERT_EQUAL(r->Submatches.size(), 3);
    UNIT_ASSERT_EQUAL(r->MatchedId, 1);

    UNIT_ASSERT_EQUAL(r->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(r->Submatches[0].second, 1);

    UNIT_ASSERT_EQUAL(r->Submatches[1].first, 1);
    UNIT_ASSERT_EQUAL(r->Submatches[1].second, 3);

    UNIT_ASSERT_EQUAL(r->Submatches[2].first, 3);
    UNIT_ASSERT_EQUAL(r->Submatches[2].second, 4);

    r = MatchCombined(v, "a c");

    UNIT_ASSERT_EQUAL(r->Submatches.size(), 3);
    UNIT_ASSERT_EQUAL(r->MatchedId, 1);

    UNIT_ASSERT_EQUAL(r->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(r->Submatches[0].second, 1);

    UNIT_ASSERT_EQUAL(r->Submatches[1].first, size_t(-1));
    UNIT_ASSERT_EQUAL(r->Submatches[1].second, size_t(-1));

    UNIT_ASSERT_EQUAL(r->Submatches[2].first, 1);
    UNIT_ASSERT_EQUAL(r->Submatches[2].second, 2);

    r = MatchCombined(v, "a b c");

    UNIT_ASSERT_EQUAL(r->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(r->MatchedId, 0);

    UNIT_ASSERT_EQUAL(r->Submatches[0].first, 2);
    UNIT_ASSERT_EQUAL(r->Submatches[0].second, 3);
}

void RemorphCoreRe::MatchAll1() {
    // TDBGOFlagsGuard g(DO_MATCH);
    TString re = "(aB)c|a(bc)";
    TString input = "abc";
    TVectorTokens tokens;
    TLiteralTableAscii lt;
    TVector<char> inputV(input.size());
    memcpy(&inputV[0], input.data(), input.size());
    Parse(tokens, re);
    TDFAPtr dfa(CompileDFA(lt, tokens));
    // {
    //     TUnbufferedFileOutput out("match_all_dfa.dot");
    //     PrintAsDot(out, lt, *dfa);
    // }
    TVector<TMatchInfoPtr> matches;
    MatchAllText(lt, *dfa, NRemorph::TVectorIterator<char>(inputV), matches);
    Cdbg << matches.size() << Endl;
    UNIT_ASSERT_EQUAL(matches.size(), 2);
    Cdbg << matches[0]->Submatches << Endl;
    Cdbg << matches[1]->Submatches << Endl;
    UNIT_ASSERT_EQUAL(matches[0]->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[0].first, size_t(-1));
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[0].second, size_t(-1));
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[1].first, 1);
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[1].second, 3);
    UNIT_ASSERT_EQUAL(matches[1]->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(matches[1]->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(matches[1]->Submatches[0].second, 2);
    UNIT_ASSERT_EQUAL(matches[1]->Submatches[1].first, size_t(-1));
    UNIT_ASSERT_EQUAL(matches[1]->Submatches[1].second, size_t(-1));
}

void RemorphCoreRe::MatchAll2() {
    TVector<TString> v;
    v.push_back("(aB)c");
    v.push_back("a(bc)");
    TString input("abc");
    TVector<TMatchInfoPtr> matches;
    MatchAllCombined(v, input, "", matches);
    Cdbg << matches.size() << Endl;
    UNIT_ASSERT_EQUAL(matches.size(), 2);
    Cdbg << matches[0]->Submatches << Endl;
    Cdbg << matches[1]->Submatches << Endl;
    UNIT_ASSERT_EQUAL(matches[0]->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[0].first, 1);
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[0].second, 3);
    UNIT_ASSERT_EQUAL(matches[1]->Submatches.size(), 1);
    UNIT_ASSERT_EQUAL(matches[1]->Submatches[0].first, 0);
    UNIT_ASSERT_EQUAL(matches[1]->Submatches[0].second, 2);
}

void RemorphCoreRe::SearchAll() {
    TDBGOFlagsGuard g(DO_MATCH);
    TString re = "(aB)c|a(bc)";
    TString input = "qqabcqqq";
    TVectorTokens tokens;
    TLiteralTableAscii lt;
    TVector<char> inputV(input.size());
    memcpy(&inputV[0], input.data(), input.size());
    Parse(tokens, re);
    TDFAPtr dfa(CompileDFA(lt, tokens));
    // {
    //     TUnbufferedFileOutput out("TestSearchAll.dot");
    //     PrintAsDot(out, lt, *dfa);
    // }
    TVector<TMatchInfoPtr> matches;
    SearchAllText(lt, *dfa, NRemorph::TVectorIterator<char>(inputV), matches);
    Cdbg << matches.size() << Endl;
    UNIT_ASSERT_EQUAL(matches.size(), 2);
    Cdbg << matches[0]->Submatches << Endl;
    Cdbg << matches[1]->Submatches << Endl;
    UNIT_ASSERT_EQUAL(matches[0]->Submatches.size(), 2);
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[0].first, size_t(-1));
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[0].second, size_t(-1));
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[1].first, 3);
    UNIT_ASSERT_EQUAL(matches[0]->Submatches[1].second, 5);
    UNIT_ASSERT_EQUAL(matches[1]->Submatches.size(), 2);

    UNIT_ASSERT_EQUAL(matches[1]->Submatches[0].first, 2);

    UNIT_ASSERT_EQUAL(matches[1]->Submatches[0].second, 4);
    UNIT_ASSERT_EQUAL(matches[1]->Submatches[1].first, size_t(-1));
    UNIT_ASSERT_EQUAL(matches[1]->Submatches[1].second, size_t(-1));
}
