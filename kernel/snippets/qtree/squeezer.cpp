#include "squeezer.h"
#include "query.h"

namespace NSnippets {
    void TQueryPosSqueezer::SqueezeWeights(const TQueryy& query)
    {
        const size_t posCount = static_cast<size_t>(query.PosCount());
        const size_t userPosCount = static_cast<size_t>(query.UserPosCount());
        Pos2SqueezedPos.reserve(posCount);
        if (posCount <= Threshold) {
            Weights.reserve(posCount);
            for (size_t i = 0; i < posCount; ++i) {
                Weights.push_back(query.Positions[i].Idf);
                Pos2SqueezedPos.push_back(i);
            }
        } else {
            Weights.reserve(userPosCount);
            size_t userPosIndex = 0;
            size_t lastOrigUserPosIndex = 0;
            for (size_t i = 0; i < posCount; ++i) {
                bool isPhonePos = query.Positions[i].WordType == QWT_PHONE;
                if (query.Positions[i].IsUserWord) {
                    Weights.push_back(query.Positions[i].Idf);
                    if (!isPhonePos) {
                        lastOrigUserPosIndex = userPosIndex;
                    }
                    ++userPosIndex;
                }
                // TODO: review the case isPhonePos == true
                Pos2SqueezedPos.push_back(isPhonePos ? 0 : lastOrigUserPosIndex);
            }
        }
    }

    TQueryPosSqueezer::TQueryPosSqueezer(const TQueryy& query, const size_t threshold)
        : Threshold(threshold)
    {
        SqueezeWeights(query);
    }

    const TVector<double>& TQueryPosSqueezer::GetSqueezedWeights() const
    {
        return Weights;
    }

    size_t TQueryPosSqueezer::GetSqueezedPosCount() const
    {
        return Weights.size();
    }

    TVector<bool> TQueryPosSqueezer::Squeeze(const TVector<bool>& seenPos) const
    {
        TVector<bool> res(Weights.size(), false);
        for (size_t i = 0; i < seenPos.size(); ++i) {
            if (seenPos[i]) {
                res[Pos2SqueezedPos[i]] = true;
            }
        }
        return res;
    }

    TVector<bool> TQueryPosSqueezer::Squeeze(const TVector<int>& seenPos) const
    {
        TVector<bool> res(Weights.size(), false);
        for (size_t i = 0; i < seenPos.size(); ++i) {
            if (seenPos[i] > 0) {
                res[Pos2SqueezedPos[i]] = true;
            }
        }
        return res;
    }

    size_t TQueryPosSqueezer::GetThreshold() const
    {
        return Threshold;
    }
}
