#include "weighted_levenstein.h"
#include "word_lemma_pair.h"

#include <kernel/lemmer/dictlib/tgrammar_processing.h>

#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>

namespace {

    constexpr float WordFormChangeWeight = 0.2f;

    constexpr size_t FirstPartOfSpeech = static_cast<size_t>(gFirst);
    constexpr size_t LastPartOfSpeech = static_cast<size_t>(gLastPartOfSpeech);

    constexpr float PosDeleteInsertWeightDefault = 1.0f;
    constexpr float PosDeleteInsertWeightParticiple = 0.55f;
    const float PosDeleteInsertWeights[LastPartOfSpeech - FirstPartOfSpeech + 1] = {
        0.3f, // gFirst = gPostposition,
        0.55f,// gAdjective
        0.4f, // gAdverb
        0.4f, // gComposite
        0.2f, // gConjunction
        0.15f,// gInterjunction
        0.6f, // gNumeral
        0.2f, // gParticle
        0.3f, // gPreposition
        0.65f,// gSubstantive
        1.0f, // gVerb
        0.5f, // gAdjNumeral
        0.45f,// gAdjPronoun
        0.4f, // gAdvPronoun
        0.4f, // gSubstPronoun
        0.3f, // gArticle
        0.5f, // gPartOfIdiom = gLastPartOfSpeech
    };

    EGrammar GetRussianPartOfSpeech(const TYandexLemma& wordLemma) {
        const auto stemGram = wordLemma.GetStemGram();
        if (stemGram == nullptr) {
            return gInvalid;
        }

        const auto gramTag = NTGrammarProcessing::ch2tg(stemGram[0]);
        if (gramTag < gAdjective || gramTag > gSubstPronoun) {
            return gInvalid;
        }

        if (gramTag == gVerb && wordLemma.FlexGramNum() > 1) {
            return gParticiple;
        }
        return gramTag;
    }

    EGrammar GetEnglishPartOfSpeech(const TYandexLemma& wordLemma) {
        const auto stemGram = wordLemma.GetStemGram();
        if (stemGram == nullptr) {
            return gInvalid;
        }

        const auto gramTag = NTGrammarProcessing::ch2tg(stemGram[0]);
        if (gramTag < gAdjective || gramTag > gArticle) {
            return gInvalid;
        }

        return gramTag;
    }

    EGrammar GetPartOfSpeech(const TYandexLemma& wordLemma) {
        const auto lang = wordLemma.GetLanguage();
        switch (lang) {
        case LANG_RUS:
            return GetRussianPartOfSpeech(wordLemma);
        case LANG_ENG:
            return GetEnglishPartOfSpeech(wordLemma);
        default:
            return gInvalid;
        }
    }

}


namespace NEditDistanceFeatures {

    float GetFormChangeWeight() {
        return WordFormChangeWeight;
    }

    float GetPosDeleteInsertWeight(EGrammar gram) {
        if (gram == gInvalid) {
            return PosDeleteInsertWeightDefault;
        }

        if (gram == gParticiple) {
            return PosDeleteInsertWeightParticiple;
        }

        return PosDeleteInsertWeights[static_cast<size_t>(gram) - FirstPartOfSpeech];
    }

    float GetPosReplaceWeight(EGrammar gramSrc, EGrammar gramDst) {
        return gramSrc != gramDst
            ? Max(GetPosDeleteInsertWeight(gramSrc), GetPosDeleteInsertWeight(gramDst))
            : GetPosDeleteInsertWeight(gramSrc);
    }

    struct InsertDeleteWordLemmaWeigher {
        float operator()(const TWordLemmaPair& wlPair) const {
            const auto pos = GetPartOfSpeech(wlPair.GetLemmas()[0]);
            const float scoreIncrement = GetPosDeleteInsertWeight(pos);
            return scoreIncrement;
        }
    };

    struct ReplaceWordLemmaWeigher {
        float operator()(const TWordLemmaPair& trg, const TWordLemmaPair& src) const {
            if (trg.Like(src)) {
                return WordFormChangeWeight;
            }

            const auto posDeleted = GetPartOfSpeech(src.GetLemmas()[0]);
            const auto posInserted = GetPartOfSpeech(trg.GetLemmas()[0]);
            const float scoreIncrement = GetPosReplaceWeight(posDeleted, posInserted);
            return scoreIncrement;
        }
    };

    std::pair<float, float> CalculateWeightedLevensteinDistance(const TVector<TWordLemmaPair>& lhsWordLemmas, const TVector<TWordLemmaPair>& rhsWordLemmas) {
        if (lhsWordLemmas.empty() && rhsWordLemmas.empty()) {
            return std::make_pair(0.0f, 0);
        }

        NLevenshtein::TEditChain chain;
        ReplaceWordLemmaWeigher rWeigher;
        InsertDeleteWordLemmaWeigher idWeigher;
        float result = 0.0f;
        NLevenshtein::GetEditChain(lhsWordLemmas, rhsWordLemmas, chain, &result, rWeigher, idWeigher, idWeigher);

        int leftPos = 0;
        int rightPos = 0;
        float compensatedWeight = 0.0f;
        for (const auto& cell: chain) {
            if (cell == NLevenshtein::EMT_PRESERVE) {
                compensatedWeight += idWeigher(lhsWordLemmas[leftPos]);
            } else if (cell == NLevenshtein::EMT_REPLACE && lhsWordLemmas[leftPos].Like(rhsWordLemmas[rightPos])) {
                compensatedWeight += Max(idWeigher(lhsWordLemmas[leftPos]), idWeigher(rhsWordLemmas[rightPos]));
                compensatedWeight -= WordFormChangeWeight;
            }
            MakeMove(cell, leftPos, rightPos);
        }
        return std::make_pair(result, result + compensatedWeight);
    }

}
