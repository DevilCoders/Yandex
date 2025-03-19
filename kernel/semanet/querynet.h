#pragma once
#include "semanet_impl.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
class TQueryNet {
public:
    void Load(const TString &filesPrefix);
    void ContinueList(const TVector<TUtf16String> &seedQueries, TVector<TUtf16String> *res, size_t count) const;
protected:
    struct TResult {
        TUtf16String Query;
        int Popularity;
        TVector<float> PathProbabilities;
    };
    typedef TVector<TResult> TResults;

    void QueryNeighbors(const TUtf16String &query, TResults *res, int maxResultsCount, int maxPathLength, float minProbability) const;
    void Neighbors(const TSemanetRecords &records, TResults *res, int maxResultsCount, int maxPathLength, float minProbability) const;
protected:
    TSemanetWTrie ReformulationsTrie;
    struct TQueryStats {
        int Popularity;
        enum EFlags {
            FLAG_PORN = 1,
            FLAG_NAV = 2,
            FLAG_SUSPICIOUS = 4,
        };
        int Flags;
        TQueryStats()
            : Popularity(0)
            , Flags(0) {
        }
    };
    struct TQueryWithStats {
        TUtf16String Query;
        TQueryStats Stats;
    };
    TVector<TQueryWithStats> Queries;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
