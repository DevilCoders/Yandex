#include "word_set_match.h"

#include <kernel/facts/common/normalize_text.h>

#include <util/generic/hash.h>
#include <util/generic/yexception.h>
#include <util/string/split.h>

namespace NFacts {

    template<class TSearchStrategy>
    typename TWordSetMatch<TSearchStrategy>::TWordDownCounter TWordSetMatch<TSearchStrategy>::MakeSubsetDownCounter(const THashMap<TString, TSizeType>& wordCounts) {
        TWordDownCounter result = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
        THashMap<TString, TIndexType> wordIndexMap;
        TSizeType uniqueWordCounter = 0;
        for (const auto& wordCount : wordCounts) {
            result.PerWordCountLeft[uniqueWordCounter++] = wordCount.second;
            result.TotalCountLeft += wordCount.second;
        }
        return result;
    }

    // the input sentence is split into words; the words get normalized (see kernel/facts/common/normalize_text.h)
    template<class TSearchStrategy>
    void TWordSetMatch<TSearchStrategy>::AddWordSubset(const TString& sentence, bool normalizeText/* = true*/) {
        const TVector<TString> subsetWords = StringSplitter(normalizeText ? NUnstructuredFeatures::NormalizeTextUTF8(sentence) : sentence).Split(' ').SkipEmpty();
        THashMap<TString, TSizeType> wordCounts;
        TSizeType uniqueWordCounter = 0;
        for (const auto& subsetWord: subsetWords) {
            ++wordCounts[subsetWord];
            if (++uniqueWordCounter == MaxSubsetLen) {
                break;
            }
        }
        SubsetWordCounterPatterns.push_back(MakeSubsetDownCounter(wordCounts));
        const TSizeType thisSubsetIndex = SubsetWordCounterPatterns.size() - 1;
        uniqueWordCounter = 0;
        for (const auto& wordCount : wordCounts) {
            const auto& word = wordCount.first;
            SearchStrategy.AddSubsetWord(word, {thisSubsetIndex, uniqueWordCounter++});
        }
    }

    // finalize the data structure building procedure
    template<class TSearchStrategy>
    void TWordSetMatch<TSearchStrategy>::Finalize() {
        SearchStrategy.Finalize();
    }

    // check if the sentence matches at least one subset
    template<class TSearchStrategy>
    bool TWordSetMatch<TSearchStrategy>::DoesSentenceMatch(const TString& sentence, bool normalizeText/* = true*/) const {
        return GetFirstMatchingWordSet(sentence, normalizeText);
    }

    // find first subset the sentence matches to
    template<class TSearchStrategy>
    const TVector<TString>* TWordSetMatch<TSearchStrategy>::GetFirstMatchingWordSet(const TString& sentence, bool normalizeText/* = true*/) const {
        const TVector<TString> words = StringSplitter(normalizeText ? NUnstructuredFeatures::NormalizeTextUTF8(sentence) : sentence).Split(' ').SkipEmpty();
        TVector<TWordDownCounter> currentSubsetWordDownCounters = SubsetWordCounterPatterns;
        for (const auto& word : words) {
            // decrement word counts until we reach zero
            for (const auto& indexPair : SearchStrategy.FindIndexPairs(word)) {
                const auto& subsetIndex = indexPair.SubsetIndex;
                const auto& wordIndex = indexPair.WordIndexWithinSubset;
                TSizeType& wordCounter = currentSubsetWordDownCounters[subsetIndex].PerWordCountLeft[wordIndex];
                if (wordCounter == 0) {
                    continue; // no more decrements for this word left
                }
                --wordCounter;
                TSizeType& totalCounter = currentSubsetWordDownCounters[subsetIndex].TotalCountLeft;
                if (--totalCounter == 0) {
                    return &SearchStrategy.GetWordSubset(subsetIndex); // we did it, all words in subset matched as many times as they should have
                }
            }
        }
        return nullptr;
    }

    template <class TSearchStrategy>
    bool TWordSetMatch<TSearchStrategy>::IsInitialized() const {
        return SearchStrategy.IsInitialized();
    }

    void TFullWordSearchStrategy::AddSubsetWord(const TString& word, TWordIndexPair&& indexPair) {
        Y_ENSURE(indexPair.SubsetIndex <= WordSubsets.size());
        if (indexPair.SubsetIndex == WordSubsets.size()) {
            WordSubsets.push_back(TVector<TString>());
        }
        WordSubsets[indexPair.SubsetIndex].push_back(word);
        WordsToSubsets[word].push_back(std::move(indexPair));
    }

    const TVector<TWordIndexPair>& TFullWordSearchStrategy::FindIndexPairs(const TString& word) const {
        const auto wordItr = WordsToSubsets.find(word);
        if (wordItr == WordsToSubsets.end()) {
            return EmptyResult;
        }
        return wordItr->second;
    }

    const TVector<TString>& TFullWordSearchStrategy::GetWordSubset(const std::size_t& subsetIndex) const {
        return WordSubsets[subsetIndex];
    }

    template class TWordSetMatch<TFullWordSearchStrategy>;


    void TSubstringSearchStrategy::AddSubsetWord(const TString& substring, TWordIndexPair&& indexPair) {
        Y_ENSURE(indexPair.SubsetIndex <= WordSubsets.size());
        if (indexPair.SubsetIndex == WordSubsets.size()) {
            WordSubsets.push_back(TVector<TString>());
        }
        WordSubsets[indexPair.SubsetIndex].push_back(substring);
        Builder.AddString(substring, {indexPair.SubsetIndex, indexPair.WordIndexWithinSubset});
    }

    void TSubstringSearchStrategy::Finalize() {
        SubstringsToSubsets.Reset(new TMappedAhoCorasick<TString, std::pair<std::size_t, std::size_t>>(Builder.Save()));
    }

    const TVector<TWordIndexPair> TSubstringSearchStrategy::FindIndexPairs(const TString& word) const {
        Y_ENSURE(SubstringsToSubsets.Get() != nullptr, "Please finalize the strategy before searching");
        const auto ahoResult = SubstringsToSubsets->AhoSearch(word);
        TVector<TWordIndexPair> result;
        result.reserve(ahoResult.size());
        for (const auto& aho : ahoResult) {
            result.push_back(TWordIndexPair{aho.second.first, aho.second.second});
        }
        return result;
    }

    const TVector<TString>& TSubstringSearchStrategy::GetWordSubset(const std::size_t& subsetIndex) const {
        return WordSubsets[subsetIndex];
    }

    bool TSubstringSearchStrategy::IsInitialized() const {
        return (SubstringsToSubsets.Get() != nullptr);
    }

    template class TWordSetMatch<TSubstringSearchStrategy>;

}
