#pragma once

#include <library/cpp/on_disk/aho_corasick/writer.h>
#include <library/cpp/on_disk/aho_corasick/reader.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NFacts {

    // the first index of the pair is the index of the subset containing the word
    // the second index is the index of this word in this subset
    struct TWordIndexPair {
        std::size_t SubsetIndex;
        std::size_t WordIndexWithinSubset;
    };

    // This class checks if an input string contains every word from at least one set of words
    template<class TSearchStrategy>
    class TWordSetMatch final {
    public:
        using TSizeType = std::size_t;
        using TIndexType = std::size_t;
        using TSearchType = TSearchStrategy;

        static constexpr TSizeType MaxSubsetLen = 8;

    public:
        // the input sentence is split into words; the words get normalized (see kernel/facts/common/normalize_text.h)
        void AddWordSubset(const TString& sentence, bool normalizeText = true);

        // finalize the data structure building procedure
        void Finalize();

        // check if the sentence matches at least one subset
        bool DoesSentenceMatch(const TString& sentence, bool normalizeText = true) const;

        // check for correct initialization
        bool IsInitialized() const;

        // find first subset the sentence matches to
        const TVector<TString>* GetFirstMatchingWordSet(const TString& sentence, bool normalizeText = true) const;

    private:
        // this structure is used to track the set matching
        struct TWordDownCounter {
            TSizeType TotalCountLeft;
            TIndexType PerWordCountLeft[MaxSubsetLen];
        };

    private:
        static TWordDownCounter MakeSubsetDownCounter(const THashMap<TString, TSizeType>& wordCounts);

    private:
        TSearchType SearchStrategy;
        TVector<TWordDownCounter> SubsetWordCounterPatterns; // contains initial states for every subset's down-counter
    };


    // The following strategy searches for full word matches
    class TFullWordSearchStrategy {
    public:
        void AddSubsetWord(const TString& word, TWordIndexPair&& indexPair);
        void Finalize() {}
        bool IsInitialized() const { return true; }
        const TVector<TWordIndexPair>& FindIndexPairs(const TString& word) const;
        const TVector<TString>& GetWordSubset(const std::size_t& subsetIndex) const;

    private:
        THashMap<TString, TVector<TWordIndexPair>> WordsToSubsets; // a word is mapped to a list of index pairs
        TVector<TVector<TString>> WordSubsets;
        const TVector<TWordIndexPair> EmptyResult;
    };

    // This strategy searches for substring matches
    class TSubstringSearchStrategy {
    public:
        void AddSubsetWord(const TString& substring, TWordIndexPair&& indexPair);
        void Finalize();
        bool IsInitialized() const;
        const TVector<TWordIndexPair> FindIndexPairs(const TString& word) const;
        const TVector<TString>& GetWordSubset(const std::size_t& subsetIndex) const;

    private:
        TAhoCorasickBuilder<TString, std::pair<std::size_t, std::size_t>> Builder;
        THolder<TMappedAhoCorasick<TString, std::pair<std::size_t, std::size_t>>> SubstringsToSubsets; // a substring is mapped to a list of index pairs
        TVector<TVector<TString>> WordSubsets;
    };

    // These templates are explicitly instantiated in the source file
    using TWordSetStrictMatch = TWordSetMatch<TFullWordSearchStrategy>;
    using TWordSetSubstringMatch = TWordSetMatch<TSubstringSearchStrategy>;

}
