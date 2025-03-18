#include <library/cpp/regex/hyperscan/hyperscan.h>
#include <library/cpp/regex/pire/pire.h>
#include <library/cpp/regex/pire2hyperscan/pire2hyperscan.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE() {
    using namespace NHyperscan;

    Y_UNIT_TEST(CompileSimpleRegex) {
        TString pireRe = "a.c";
        TString hyperscanRe = PireRegex2Hyperscan(pireRe);
        TDatabase db = Compile(hyperscanRe, HS_FLAG_DOTALL);
        TScratch scratch = MakeScratch(db);
        UNIT_ASSERT(NHyperscan::Matches(db, scratch, "abc"));
        UNIT_ASSERT(!NHyperscan::Matches(db, scratch, "foo"));
    }

    enum EResult {
        MATCHES,
        NOT_MATCHES,
        NOT_IMPLEMENTED,
        POOR_PATTERN,
    };

    EResult MatchesWith(TString pireRe, TString text) {
        TString hyperscanRe;
        try {
            hyperscanRe = PireRegex2Hyperscan(pireRe);
        } catch (NHyperscan::TCompileException) {
            return NOT_IMPLEMENTED;
        } catch (...) {
            return POOR_PATTERN;
        }
        TDatabase db = Compile(hyperscanRe, HS_FLAG_DOTALL);
        TScratch scratch = MakeScratch(db);
        return NHyperscan::Matches(db, scratch, text) ? MATCHES : NOT_MATCHES;
    }

    EResult MatchesWith(NPire::TLexer & lexer, TString text) {
        TString hyperscanRe;
        try {
            hyperscanRe = PireLexer2Hyperscan(lexer);
        } catch (NHyperscan::TCompileException) {
            return NOT_IMPLEMENTED;
        } catch (...) {
            return POOR_PATTERN;
        }
        TDatabase db = Compile(hyperscanRe, HS_FLAG_DOTALL);
        TScratch scratch = MakeScratch(db);
        return NHyperscan::Matches(db, scratch, text) ? MATCHES : NOT_MATCHES;
    }

    Y_UNIT_TEST(MoreRegexs) {
        UNIT_ASSERT_EQUAL(MatchesWith("a.c", "abc"), MATCHES);
        UNIT_ASSERT_EQUAL(MatchesWith("a.c", "foo"), NOT_MATCHES);
        UNIT_ASSERT_EQUAL(MatchesWith("a.+c", "abc"), MATCHES);
        UNIT_ASSERT_EQUAL(MatchesWith("[^4]submit", "submit"), MATCHES);
        UNIT_ASSERT_EQUAL(MatchesWith("[^4]submit", "5submit"), MATCHES);
        UNIT_ASSERT_EQUAL(MatchesWith("[^4]submit", "4submit"), NOT_MATCHES);
        UNIT_ASSERT_EQUAL(MatchesWith("submit$", "submit"), MATCHES);
        UNIT_ASSERT_EQUAL(MatchesWith("submit$", "submit!"), NOT_MATCHES);
        UNIT_ASSERT_EQUAL(MatchesWith("[フェイト／ゼロ]", "ェ"), MATCHES);
    }

    Y_UNIT_TEST(AndNotOperationThrowsTHyperscanCompileException) {
        UNIT_ASSERT_EQUAL(MatchesWith("a&b", ""), NOT_IMPLEMENTED);
        UNIT_ASSERT_EQUAL(MatchesWith("a\\&b", "a&b"), MATCHES);
        UNIT_ASSERT_EQUAL(MatchesWith("~a", ""), NOT_IMPLEMENTED);
        UNIT_ASSERT_EQUAL(MatchesWith("\\~a", "~a"), MATCHES);
    }

    Y_UNIT_TEST(ThrowsOnPoorPatterns) {
        UNIT_ASSERT_EQUAL(MatchesWith("[", ""), POOR_PATTERN);
    }

    Y_UNIT_TEST(PireLexer2Hyperscan) {
        TString regex = "foo";
        std::vector<wchar32> ucs4;
        NPire::NEncodings::Utf8().FromLocal(
            regex.begin(),
            regex.end(),
            std::back_inserter(ucs4));
        NPire::TLexer lexer(ucs4.begin(), ucs4.end());
        lexer.AddFeature(NPire::NFeatures::AndNotSupport());
        lexer.AddFeature(NPire::NFeatures::CaseInsensitive());
        UNIT_ASSERT_EQUAL(MatchesWith(lexer, "FOO"), MATCHES);
    }
}
