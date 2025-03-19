#pragma once

#include "word_lemma_pair.h"

#include <kernel/lemmer/dictlib/grammar_enum.h>

#include <util/generic/vector.h>

#include <utility>

namespace NEditDistanceFeatures {

    /// @brief Finds lemma duplicates and applies word-lemma transpositions to the left-hand sequence to make the sequences of the duplicates equal.
    ///        FACTS-1770: the only supported languages yet are Russian and English.
    /// @param[in, out] lhsWords Left-hand word-lemma sequence, can be modified
    /// @param[in] rhsWords Right-hand word-lemma sequence
    /// @return Pair of numbers: number of transpositions made, number of lemma duplicates found
    std::pair<size_t, size_t> CountAndMakeWordLemmaTranspositions(TVector<TWordLemmaPair>& lhsWords, const TVector<TWordLemmaPair>& rhsWords);

}
