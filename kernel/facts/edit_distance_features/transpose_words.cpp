#include "transpose_words.h"

#include <kernel/lemmer/core/lemmeraux.h>

#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>

#include <util/generic/map.h>

#include <limits>

namespace {

    struct WordPosPair {
        NEditDistanceFeatures::TWordLemmaPair WordLemma;
        size_t Pos;

        bool operator==(const WordPosPair& rhs) const {
            return this->WordLemma == rhs.WordLemma;
        }

        bool operator<(const WordPosPair& rhPair) const {
            return this->Pos < rhPair.Pos;
        };
    };

    bool WordLemmaLess(const NEditDistanceFeatures::TWordLemmaPair& lhs, const NEditDistanceFeatures::TWordLemmaPair& rhs) {
        bool firstLemmaLess = false;
        bool firstLemmaCompared = false;
        for (const auto& lhLemma: lhs.GetLemmas()) {
            for (const auto& rhLemma: rhs.GetLemmas()) {
                const auto cmpResult = lhLemma.GetTextBuf().compare(rhLemma.GetTextBuf());
                if (cmpResult != 0) {
                    if (!firstLemmaCompared) {
                        firstLemmaLess = cmpResult < 0;
                    }
                    continue;
                }

                if (NLemmerAux::TYandexLemmaGetter(lhLemma).GetRuleId() == NLemmerAux::TYandexLemmaGetter(rhLemma).GetRuleId()) {
                    return false;
                }
            }
        }
        return firstLemmaLess;
    }

}


namespace NEditDistanceFeatures {

    std::pair<size_t, size_t> CountAndMakeWordLemmaTranspositions(TVector<TWordLemmaPair>& lhsWords, const TVector<TWordLemmaPair>& rhsWords) {
        using LeftRightPos = std::pair<size_t, size_t>;
        using WordPosMap = TMultiMap<TWordLemmaPair, LeftRightPos, decltype(&WordLemmaLess)>;
        constexpr size_t invalidPos = std::numeric_limits<size_t>::max();

        WordPosMap wordPosMap(&WordLemmaLess);
        for (size_t i = 0; i < rhsWords.size(); ++i) {
            wordPosMap.insert(std::make_pair(rhsWords[i], std::make_pair(invalidPos, i)));
        }

        size_t duplicateWordCount = 0;
        for (size_t i = 0; i < lhsWords.size(); ++i) {
            WordPosMap::iterator equalItr;
            WordPosMap::const_iterator equalEnd;
            std::tie(equalItr, equalEnd) = wordPosMap.equal_range(lhsWords[i]);
            for (; equalItr != equalEnd; ++equalItr) {
                auto& lhPosRef = equalItr->second.first;
                if (lhPosRef == invalidPos) {
                    lhPosRef = i;
                    ++duplicateWordCount;
                    break; // mark one instance only
                }
            }
        }

        if (duplicateWordCount <= 1) {
            return std::make_pair(0, duplicateWordCount);
        }

        using WordPosVector = TVector<WordPosPair>;
        WordPosVector wordsSortedLh;
        wordsSortedLh.reserve(duplicateWordCount);
        WordPosVector wordsSortedRh;
        wordsSortedRh.reserve(duplicateWordCount);
        auto mapItr = wordPosMap.cbegin();
        const auto mapEnd = wordPosMap.cend();
        for (; mapItr != mapEnd; ++mapItr) {
            if (mapItr->second.first != invalidPos) {
                wordsSortedLh.push_back(WordPosPair{mapItr->first, mapItr->second.first});
                wordsSortedRh.push_back(WordPosPair{mapItr->first, mapItr->second.second});
            }
        }

        Sort(wordsSortedLh);
        Sort(wordsSortedRh);

        size_t wordsTransposedCounter = 0;

        auto replacePreventWeigher = [](const WordPosPair&, const WordPosPair&) -> float {
            return 1e6f;
        };
        NLevenshtein::TEditChain chain;
        float minWeight = 0.0f;
        NLevenshtein::GetEditChain(wordsSortedLh, wordsSortedRh, chain, &minWeight, replacePreventWeigher);

        if (!chain.empty()) {
            // applying the edit chain to the left-hand word sequence
            TVector<TWordLemmaPair> reorderedLhsWords;
            reorderedLhsWords.reserve(lhsWords.size());
            size_t insertDeleteCount = 0;
            size_t prevTrgPos = 0;
            size_t lhPos = 0;
            size_t rhPos = 0;
            for (const auto& cell: chain) {
                // append intermediate non-duplicate lemmas
                for (size_t i = prevTrgPos; i < wordsSortedLh[Min(lhPos, wordsSortedLh.size() - 1)].Pos; ++i) {
                    reorderedLhsWords.push_back(lhsWords[i]);
                }

                prevTrgPos = (lhPos < wordsSortedLh.size() ? wordsSortedLh[lhPos].Pos : wordsSortedLh.back().Pos) + 1;

                switch (cell) {
                case NLevenshtein::EMT_PRESERVE:
                    reorderedLhsWords.push_back(lhsWords[wordsSortedLh[lhPos].Pos]);
                    prevTrgPos = wordsSortedLh[lhPos].Pos + 1;
                    ++lhPos;
                    ++rhPos;
                    break;
                case NLevenshtein::EMT_DELETE:
                    ++insertDeleteCount;
                    prevTrgPos = wordsSortedLh[lhPos].Pos + 1;
                    ++lhPos;
                    break;
                case NLevenshtein::EMT_INSERT:
                    ++insertDeleteCount;
                    {
                        const auto rhWord = rhsWords[wordsSortedRh[rhPos].Pos];
                        WordPosMap::const_iterator equalItr;
                        WordPosMap::const_iterator equalEnd;
                        std::tie(equalItr, equalEnd) = wordPosMap.equal_range(rhWord);
                        for (; equalItr != equalEnd; ++equalItr) {
                            if (equalItr->second.second == wordsSortedRh[rhPos].Pos) {
                                reorderedLhsWords.push_back(lhsWords[equalItr->second.first]);
                            }
                        }
                    }
                    ++rhPos;
                    break;
                case NLevenshtein::EMT_REPLACE:
                    ++lhPos;
                    ++rhPos;
                    break;
                default:
                    break;
                }
            }

            for (size_t i = prevTrgPos; i < lhsWords.size(); ++i) {
                reorderedLhsWords.push_back(lhsWords[i]);
            }

            wordsTransposedCounter = insertDeleteCount / 2;
            lhsWords.swap(reorderedLhsWords);
        }
        return std::make_pair(wordsTransposedCounter, duplicateWordCount);
    }

}
