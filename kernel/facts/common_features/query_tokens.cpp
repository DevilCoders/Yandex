#include "query_tokens.h"

#include <library/cpp/tokenizer/tokenizer.h>
#include <kernel/facts/features_calculator/analyzed_word/analyzed_word.h>
#include <search/web/util/lemmer_cache/lemmer_cache.h>

namespace NFacts {
    static const TUtf16String INTEGER = u"INTEGER";
    static const TUtf16String REAL = u"REAL";

    using namespace NUnstructuredFeatures;


    /////////////////////////////////////////////
    TUtf16String SubstNumericTokenOrGetLemma(const TTypedLemmedToken& token) {
        int integer;
        if (token.Type == NLP_TYPE::NLP_INTEGER && TryFromString(token.Word, integer)) {
            return INTEGER;
        }
        float real;
        if (token.Type == NLP_TYPE::NLP_FLOAT && TryFromString(WideToUTF8(token.Word), real)) {
            return REAL;
        }
        return token.BestLemma.GetText();
    }


    /////////////////////////////////////////////
    TYandexLemma TTypedLemmedToken::GetBestLemma(const TUtf16String& word, const NLP_TYPE& type, TWLemmaArray& lemmas) {
        static const auto& LANGS = TLangMask(LANG_RUS, LANG_ENG);

        if (type != NLP_TYPE::NLP_WORD && type != NLP_TYPE::NLP_MARK) {
            return TYandexLemma();
        }

        TYandexLemma* bestLemma = nullptr;
        NLemmerCache::AnalyzeWord(word.c_str(), word.length(), lemmas, LANGS);
        double bestLemmaWeight = 0;
        for (auto& lemmerResult : lemmas) {
            if (lemmerResult.GetWeight() > bestLemmaWeight) {
                bestLemmaWeight = lemmerResult.GetWeight();
                bestLemma = &lemmerResult;
            }
        }

        return (bestLemma == nullptr ? TYandexLemma() : std::move(*bestLemma));
    }
}
