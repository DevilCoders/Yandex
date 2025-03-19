#include "querynet.h"
#include <library/cpp/containers/ext_priority_queue/ext_priority_queue.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/charset/wide.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
void TQueryNet::Load(const TString &filesPrefix) {
    ReformulationsTrie.Init(TBlob::FromFileContent(filesPrefix + ".reftrie"));
    {
        TFileInput qfile(filesPrefix + ".queries");
        TString q;
        TVector<TStringBuf> tabs;
        while (qfile.ReadLine(q)) {
            Split(q.begin(), '\t', &tabs);
            Queries.emplace_back();
            Queries.back().Query = UTF8ToWide(tabs[0]);
            Queries.back().Stats.Popularity = FromString<int>(tabs[1]);
            for (size_t n = 2; n < tabs.size(); ++n) {
                if (tabs[n] == "porn")
                    Queries.back().Stats.Flags |= TQueryStats::FLAG_PORN;
                else if (tabs[n] == "nav")
                    Queries.back().Stats.Flags |= TQueryStats::FLAG_NAV;
                else if (tabs[n] == "susp")
                    Queries.back().Stats.Flags |= TQueryStats::FLAG_SUSPICIOUS;
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TQueryNet::QueryNeighbors(const TUtf16String &query, TResults *res, int maxResultsCount, int maxPathLength, float minProbability) const {
    TSemanetRecords records;
    if (ReformulationsTrie.Find(query, &records))
        Neighbors(records, res, maxResultsCount, maxPathLength, minProbability);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TQueryNet::Neighbors(const TSemanetRecords &records, TResults *res, int maxResultsCount, int maxPathLength, float minProbability) const {
    TExtPriorityQueue<TResult, float, true> topResults;
    THashSet<TUtf16String> added;
    for (TSemanetRecords::iterator it = records.begin(); it != records.end(); ++it) {
        if (it->Probability < minProbability)
            continue;
        TResult res;
        res.Query = Queries[it->TargetId].Query;
        res.Popularity = Queries[it->TargetId].Stats.Popularity;
        res.PathProbabilities.push_back(it->Probability);
        topResults.push(res, it->Probability);
        added.insert(res.Query);
    }
    while (res->ysize() < maxResultsCount && !topResults.empty()) {
        res->push_back(topResults.ytop());
        TUtf16String query = topResults.ytop().Query;
        TVector<float> path = topResults.ytop().PathProbabilities;
        topResults.pop();
        if (path.ysize() < maxPathLength) {
            TSemanetRecords furtherRecords;
            if (!ReformulationsTrie.Find(query, &furtherRecords))
                continue;
            float probabilitySoFar = 1.0f;
            for (size_t n = 0; n < path.size(); ++n)
                probabilitySoFar *= path[n];
            for (TSemanetRecords::iterator it = furtherRecords.begin(); it != furtherRecords.end(); ++it) {
                float p = probabilitySoFar * it->Probability;
                if (p < minProbability)
                    continue;
                TResult res;
                res.Popularity = Queries[it->TargetId].Stats.Popularity;
                res.Query = Queries[it->TargetId].Query;
                if (added.contains(res.Query))
                    continue;
                added.insert(res.Query);
                res.PathProbabilities = path;
                res.PathProbabilities.push_back(it->Probability);
                topResults.push(res, p);
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TrackerToResult(TSemanetTracker<TUtf16String> *tracker, TVector<TUtf16String> *res, size_t count) {
    while (res->size() < count && !tracker->Empty()) {
        TUtf16String next = tracker->GetMostFrequent();
        res->push_back(next);
        tracker->Remove(next);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TQueryNet::ContinueList(const TVector<TUtf16String> &seedQueries, TVector<TUtf16String> *res, size_t count) const {
    TSemanetTracker<TUtf16String> tracker;
    for (size_t n = 0; n < seedQueries.size(); ++n) {
        TResults results;
        QueryNeighbors(seedQueries[n], &results, 1000, 2, 1e-6f);
        for (size_t k = 0; k < results.size(); ++k) {
            TUtf16String reformulation = results[k].Query;
            float p = 1;
            for (size_t m = 0; m < results[k].PathProbabilities.size(); ++m)
                p *= results[k].PathProbabilities[m];
            tracker.Inc(reformulation, 1 + p);
        }
    }
    TrackerToResult(&tracker, res, count);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
