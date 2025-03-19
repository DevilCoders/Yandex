#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <kernel/indexer/direct_text/fl.h>
#include <ysite/yandex/pure/pure_container.h>
#include "lemcache.h"

//#define PRINT_TOKENS

using namespace NIndexerCore;
using namespace NIndexerCorePrivate;

class TLemmatizationCacheTest : public TTestBase {
    UNIT_TEST_SUITE(TLemmatizationCacheTest);
        UNIT_TEST(Test1);
        UNIT_TEST(Test2);
    UNIT_TEST_SUITE_END();
public:
    void Test1();
    void Test2();
private:
    bool FindForm(TLemmatizedTokens** seq, ui32 n, const char* form, ui8 flags, ui8 joins) {
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < seq[i]->TokenCount; ++j) {
                const TLemmatizedToken& tok = seq[i]->Tokens[j];
                if (strcmp(tok.FormaText, form) == 0 && tok.Flags == flags && tok.Joins == joins)
                    return true;
            }
        }
        return false;
    }
    bool FindLemma(TLemmatizedTokens** seq, ui32 n, const char* lemma) {
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < seq[i]->TokenCount; ++j) {
                if (strcmp(seq[i]->Tokens[j].LemmaText, lemma) == 0)
                    return true;
            }
        }
        return false;
    }
    class TTokenizer : private ITokenHandler {
        TUtf16String TokenText;
        TWideToken* Token;
        void OnToken(const TWideToken& token, size_t /* origleng */, NLP_TYPE /*type*/) override {
            if (Token == nullptr)
                yexception() << "second token found";
            TokenText = token.Text();
            *Token = token;
            Token->Token = TokenText.c_str();
            Token = nullptr;
        }
    public:
        TTokenizer() : Token(nullptr) {}
        void Tokenize(const TUtf16String& text, TWideToken& tok) {
            Token = &tok;
            TNlpTokenizer tokenizer(*this, false);
            tokenizer.Tokenize(text.c_str(), text.size(), false);
        }
    };
};

UNIT_TEST_SUITE_REGISTRATION(TLemmatizationCacheTest)

void TLemmatizationCacheTest::Test1() {
    TDTCreatorConfig config;
    TCompatiblePureContainer pure;
    pure.Load(config.PureLangConfigFile, config.PureTrieFile);
    TLemmatizationCache cache(config, &pure, TIndexLanguageOptions(LANG_UNK), &NLemmer::TAnalyzeWordOpt::IndexerOpt(), 1, config.CacheBlockSize);
    cache.SetLang(LANG_ENG);
    TWideToken tok;
    TTokenizer tokenizer;

    const TUtf16String text(u"1black-and-white2Mother's3+");
    tokenizer.Tokenize(text, tok);
    TTempArray<TLemmatizedTokens*> buf(tok.SubTokens.size());
    TLemmatizedTokens** tokens = buf.Data();
    const ui32 n = cache.StoreMultiToken(tok, 0, tokens);

#ifdef PRINT_TOKENS
    for (ui32 i = 0; i < n; ++i) {
        TLemmatizedTokens* p = tokens[i];
        for (ui32 j = 0; j < p->TokenCount; ++j) {
            const TLemmatizedToken& lt = p->Tokens[j];
            Cout << "'" << lt.LemmaText << "' '" << lt.FormaText << "' " << (int)lt.Flags << " " << (int)lt.Joins << Endl;
        }
    }
#endif

    UNIT_ASSERT(FindForm(tokens, n, "black", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "and", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "white", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN));
    UNIT_ASSERT(FindForm(tokens, n, "black-and-white", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN));
    UNIT_ASSERT(FindForm(tokens, n, "mother's", FORM_TITLECASE | FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN));

    UNIT_ASSERT(FindForm(tokens, n, "1", FORM_HAS_JOINS, FORM_RIGHT_JOIN));
    UNIT_ASSERT(FindForm(tokens, n, "2", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN));
    UNIT_ASSERT(FindForm(tokens, n, "3+", FORM_HAS_JOINS, FORM_LEFT_JOIN));

    UNIT_ASSERT(FindLemma(tokens, n, "00000000001"));
    UNIT_ASSERT(FindLemma(tokens, n, "00000000002"));
    UNIT_ASSERT(FindLemma(tokens, n, "00000000003+"));
}

void TLemmatizationCacheTest::Test2() {
    TDTCreatorConfig config;
    TCompatiblePureContainer pure;
    pure.Load(config.PureLangConfigFile, config.PureTrieFile);
    TLemmatizationCache cache(config, &pure, TIndexLanguageOptions(LANG_UNK), &NLemmer::TAnalyzeWordOpt::IndexerOpt(), 1, config.CacheBlockSize);
    cache.SetLang(LANG_ENG);
    TWideToken tok;
    TTokenizer tokenizer;

    const TUtf16String text(u"I_have_@one_thousand_dollars_$1000_a-lot-of-money");
    tokenizer.Tokenize(text, tok);
    TTempArray<TLemmatizedTokens*> buf(tok.SubTokens.size());
    TLemmatizedTokens** tokens = buf.Data();
    const ui32 n = cache.StoreMultiToken(tok, 0, tokens);

#ifdef PRINT_TOKENS
    for (ui32 i = 0; i < n; ++i) {
        TLemmatizedTokens* p = tokens[i];
        for (ui32 j = 0; j < p->TokenCount; ++j) {
            const TLemmatizedToken& lt = p->Tokens[j];
            Cout << "'" << lt.LemmaText << "' '" << lt.FormaText << "' " << (int)lt.Flags << " " << (int)lt.Joins << Endl;
        }
    }
#endif

    UNIT_ASSERT(FindForm(tokens, n, "i", FORM_TITLECASE | FORM_HAS_JOINS, FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "have", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "one", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "thousand", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "dollars", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "a", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "lot", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "of", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindForm(tokens, n, "money", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM));
//    UNIT_ASSERT(FindForm(tokens, n, "one_thousand_dollars", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));

    UNIT_ASSERT(FindForm(tokens, n, "1000", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM));
    UNIT_ASSERT(FindLemma(tokens, n, "00000001000"));
}
