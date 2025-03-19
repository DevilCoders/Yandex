#pragma once

#include <kernel/extended_mx_calcer/proto/typedefs.h>

namespace NExtendedMx {
    struct TDebug;

    const TStringBuf FEAT_POS = "Pos";
    const TStringBuf FEAT_VIEWTYPE = "ViewType";
    const TStringBuf FEAT_PLACE = "Place";
    const TStringBuf FEAT_SHOW = "Show";
    const TStringBuf FEAT_PLACE_MAIN = "Main";
    const TStringBuf NO_SHOW_STRING_FEAT = "NO_SHOW";
    const TStringBuf FEAT_IS_RANDOM = "IsRandom";

    bool FeatureValueAvailableInContext(const TFeatureContextDictConstProto& featContext, const TStringBuf& featName, const TStringBuf& featValue, TDebug& debug);
    bool CombinationAvailableInContext(const TAvailVTCombinationsConst& availVtComb, const TStringBuf& viewType, const TStringBuf& place, ui32 pos, TDebug& debug);

    // should be sync with allowed values for TMeta::PassRawPredictionMode in proto/scheme.sc
    enum class EPassRawPredictions {
        Best    /* "best" */,
        All     /* "all" */,
    };

    // returns -1 if there is no available value with score at least minScore
    template<class TValueCont, class TBoolCont>
    int GetMaxAvailableIdxWithThreshold(const TValueCont& cmp, const TValueCont& best, const TBoolCont& availableValues,
            double minScore = 0.0, double minBestScore = 0.0)
    {
        double bestScore = minBestScore;
        int bestIdx = -1;
        int i = 0;
        auto cmpIt = cmp.begin();
        auto bestIt = best.begin();
        auto availIt = availableValues.begin();

        for (; cmpIt != cmp.end() && bestIt != best.end() && availIt != availableValues.end(); ++cmpIt, ++bestIt, ++availIt, ++i)
        {
            if (*availIt && *bestIt > bestScore && *cmpIt > minScore) {
                bestScore = *bestIt;
                bestIdx = i;
            }
        }

        return bestIdx;
    }
};
