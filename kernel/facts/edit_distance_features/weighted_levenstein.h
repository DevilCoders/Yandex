#pragma once

#include "word_lemma_pair.h"

#include <kernel/lemmer/dictlib/grammar_enum.h>

#include <util/generic/vector.h>

namespace NEditDistanceFeatures {

    float GetFormChangeWeight();
    float GetPosDeleteInsertWeight(EGrammar gram);
    float GetPosReplaceWeight(EGrammar gramSrc, EGrammar gramDst);

    /// @brief Calculates the Levenstein edit distance between two lemma sequences using weights for different operations.
    ///        Weights depend on an operation type, and lemma's part of speech.
    ///        FACTS-1770: the only supported languages yet are Russian and English.
    /// @param[in] lhsWordLemmas Left-hand word-lemma sequence
    /// @param[in] rhsWordLemmas Right-hand word-lemma sequence
    /// @return Pair of numbers: edit distance, edit chain length, in operation weights
    std::pair<float, float> CalculateWeightedLevensteinDistance(const TVector<TWordLemmaPair>& lhsWordLemmas, const TVector<TWordLemmaPair>& rhsWordLemmas);

}
