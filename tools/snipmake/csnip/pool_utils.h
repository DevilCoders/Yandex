#pragma once

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <utility>

namespace NSnippets {

    extern const TString CANDIDATE_ALGO_PREFIX;

    struct TPoolMaker {
        bool PoolGeneration;
        THashSet<TString> ProcessedQDPairs;

        TPoolMaker(bool poolGeneration);
        bool Skip(const TString& query, const TString& url);
    };

    struct TAssessorDataManager {
        using TQDPairCandidateIndices = THashMap<TString, THashMap<std::pair<TString, TString>, size_t>>;
        using TQDPairCandidatePairMarks = THashMap<TString, THashMap<std::pair<size_t, size_t>, TVector<TString>>>;
        TQDPairCandidateIndices QDPairCandidateIndices;
        TQDPairCandidatePairMarks QDPairCandidatePairMarks;

        bool IsEmpty() const;
        void Init(const TString& assessorDataFileName);
        size_t InsertCandidate(const TString& qdPair, const TString& candidate, const TString& title);
        TString GetRetextExp(const TString& query, const TString& url) const;
        TString GetCleanCandidateText(const TString& candText) const;
        TString GetQDPairText(const TString& query, const TString& url) const;
    };

    struct TReqSnip;
    typedef THashMap<TString, THashMap<size_t, TReqSnip> > TQDPairCandidates;
    struct MatrixnetDataWriter
    {
        THolder<IOutputStream> FeaturesTxtOut;
        THolder<IOutputStream> FeaturesPairsTxtOut;
        const TAssessorDataManager& AssessorDataManager;
        size_t AllMarkPairs;
        size_t ProcessedMarkPairs;
        size_t WrittenLineNumber;
        size_t QDPairIndex;

        MatrixnetDataWriter(const TAssessorDataManager& assessorDataManager);
        void Write(const TQDPairCandidates& qdPairCandidates);
        void WriteCandidate(const TString& qdPair, const TReqSnip& snippet);
        float GetWeight(int informativenessValue, int contentValue, int readabilityValue) const;
        void WriteFeaturesPairs(size_t leftCandNumber, size_t rightCandNumber, const TString& mark);
    };

}
