#include "lang_check.h"

#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/alpha/abc.h>
#include <library/cpp/tokenizer/tokenizer.h>

namespace NSnippets {
    namespace {
        bool HasTokenOfAlphabet(const TWideToken& multiToken, ELanguage language) {
            const TTokenStructure& subTokens = multiToken.SubTokens;
            if (subTokens.empty()) {
                TLangMask tokenLangs = NLemmer::ClassifyLanguageAlpha(multiToken.Token, multiToken.Leng, false);
                return tokenLangs.SafeTest(language);
            }
            for (size_t i = 0; i < subTokens.size(); ++i) {
                TLangMask subTokenLangs = NLemmer::ClassifyLanguageAlpha(multiToken.Token + subTokens[i].Pos, subTokens[i].Len, false);
                if (subTokenLangs.SafeTest(language)) {
                    return true;
                }
            }
            return false;
        }

        struct TWordsOfAlphabetCountHandler : ITokenHandler {
            ELanguage Language = LANG_UNK;
            int WordsThreshold = 0;
            int WordCount = 0;
            int ThisLangWordCount = 0;

            TWordsOfAlphabetCountHandler(ELanguage language, int wordsThreshold)
                : Language(language)
                , WordsThreshold(wordsThreshold) {
            }

            bool TooManyWordsOfAlphabet() const {
                return WordsThreshold <= 0;
            }

            double GetLanguagePortion() const {
                return 1.0 * ThisLangWordCount / WordCount;
            }

            void OnToken(const TWideToken& multiToken, size_t /*origleng*/, NLP_TYPE type) override {
                if (!TooManyWordsOfAlphabet()) {
                    if (type == NLP_WORD && HasTokenOfAlphabet(multiToken, Language)) {
                        --WordsThreshold;
                    }
                }
                if (type == NLP_WORD) {
                    ++WordCount;
                    if (HasTokenOfAlphabet(multiToken, Language)) {
                        ++ThisLangWordCount;
                    }
                }
            }
        };
    }

    bool HasTooManyWordsOfAlphabet(const TUtf16String& text, ELanguage lang, int wordsThreshold) {
        TWordsOfAlphabetCountHandler handler(lang, wordsThreshold);
        TNlpTokenizer tokenizer(handler, false);
        tokenizer.Tokenize(text);
        return handler.TooManyWordsOfAlphabet();
    }

    bool HasWordsOfAlphabet(const TUtf16String& text, ELanguage lang) {
        return HasTooManyWordsOfAlphabet(text, lang, 1);
    }

    bool HasTooManyCyrillicWords(const TUtf16String& text, int cyrWordsThreshold) {
        return HasTooManyWordsOfAlphabet(text, LANG_RUS, cyrWordsThreshold);
    }

    double MeasureFractionOfLang(const TUtf16String& text, ELanguage lang) {
        TWordsOfAlphabetCountHandler handler(lang, 0);
        TNlpTokenizer tokenizer(handler, false);
        tokenizer.Tokenize(text);
        return handler.GetLanguagePortion();
    }
}
