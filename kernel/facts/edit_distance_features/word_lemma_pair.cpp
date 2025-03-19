#include "word_lemma_pair.h"

#include <kernel/lemmer/core/lemmeraux.h>

namespace NEditDistanceFeatures {

    TWordLemmaPair TWordLemmaPair::GetCachedWordLemmaPair(const TUtf16String& word, TLemmaArrayMap& lemmaCache, const TLangMask& langs) {
        auto lemmaItr = lemmaCache.find(word);
        if (lemmaItr == lemmaCache.end()) {
            TWLemmaArray lemmas;
            NLemmer::AnalyzeWord(word.c_str(), word.length(), lemmas, langs);
            bool insertResult = false;
            std::tie(lemmaItr, insertResult) = lemmaCache.insert(std::make_pair(word, std::move(lemmas)));
        }
        return TWordLemmaPair(lemmaItr->first, lemmaItr->second);
    }

    TWordLemmaPair::TWordLemmaPair(const TUtf16String& word, TWLemmaArray& lemmas)
        : Word(word)
        , Lemmas(&lemmas)
    {
    }

    bool TWordLemmaPair::operator==(const TWordLemmaPair& rhs) const {
        return Word == rhs.Word;
    }

    bool TWordLemmaPair::operator!=(const TWordLemmaPair& rhs) const {
        return !(*this == rhs);
    }

    bool TWordLemmaPair::Like(const TWordLemmaPair& rhs) const {
        if (*this == rhs) {
            return true;
        }

        for (const auto& lhLemma: *Lemmas) {
            for (const auto& rhLemma: *rhs.Lemmas) {
                const auto cmpResult = lhLemma.GetTextBuf().compare(rhLemma.GetTextBuf());
                if (cmpResult != 0) {
                    continue;
                }

                if (NLemmerAux::TYandexLemmaGetter(lhLemma).GetRuleId() == NLemmerAux::TYandexLemmaGetter(rhLemma).GetRuleId()) {
                    return true;
                }
            }
        }
        return false;
    }

    const TUtf16String& TWordLemmaPair::GetWord() const {
        return Word;
    }

    const TWLemmaArray& TWordLemmaPair::GetLemmas() const {
        return *Lemmas;
    }

}
