#include "plm.h"
#include <kernel/snippets/sent_match/sent_match.h>

#include <kernel/snippets/qtree/query.h>

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>

namespace NSnippets
{
    static const float MIN_WORD_WEIGHT = 0.0001f;
    static const int PLM_SIGMA = 60;

    TPLMStatData::TPLMStatData(const TSentsMatchInfo& info)
      : Info(info)
      , PosterioriQueryWordWeight(info.Query.PosCount() > 0 ? 1.0 / info.Query.PosCount() : 0)
      , QueryWordPosWeight(info.Query.PosCount(), 0.0)
      , LastQueryWordPositionsInSent(info.Query.PosCount(), -1)
    {
    }

    double TPLMStatData::CalculatePLMScore(const TVector<std::pair<int, int>>& r, bool weightSumOnly)
    {
        const TQueryy& query = Info.Query;
        if (!query.PosCount()) {
            return 0;
        }

        Fill(LastQueryWordPositionsInSent.begin(), LastQueryWordPositionsInSent.end(), -1);
        double weightSum = 0;
        Fill(QueryWordPosWeight.begin(), QueryWordPosWeight.end(), 0.0);

        int shift = 0;
        for (size_t ir = 0; ir < r.size(); ++ir) {
            if (ir) {
                shift += r[ir - 1].second - r[ir - 1].first + 1;
            }
            for (int w = r[ir].first; w <= r[ir].second; ++w) {
                for (int lemmaId : Info.GetNotExactMatchedLemmaIds(w)) {
                    for (int pos : query.Id2Poss[lemmaId]) {
                        if (!query.Positions[pos].IsStopWord) {
                            if (LastQueryWordPositionsInSent[pos] < w) {
                                const int actualPosition = w - r[ir].first + shift;
                                double weight = exp(-1.0 * actualPosition * actualPosition / PLM_SIGMA);
                                QueryWordPosWeight[pos] += weight;
                                weightSum += weight;
                                LastQueryWordPositionsInSent[pos] = w;
                            }
                        }
                    }
                }
            }
        }

        if (weightSumOnly)
             return weightSum;

        double divergence = 0;
        for (size_t pos = 0; pos < QueryWordPosWeight.size(); ++pos) {
            if (QueryWordPosWeight[pos] == 0) {
                QueryWordPosWeight[pos] = MIN_WORD_WEIGHT;
            } else {
                QueryWordPosWeight[pos] = (QueryWordPosWeight[pos]) / (weightSum + MIN_WORD_WEIGHT);
            }
            divergence += log(PosterioriQueryWordWeight / QueryWordPosWeight[pos]);
        }
        return -PosterioriQueryWordWeight * divergence;
    }

    double TPLMStatData::CalculateWeightSum(const TSpans& s)
    {
        TVector<std::pair<int, int>> r;
        for (TSpanCIt it = s.begin(); it != s.end(); ++it)
            r.push_back(std::make_pair(it->First, it->Last));
        return CalculatePLMScore(r, true);
    }

}
