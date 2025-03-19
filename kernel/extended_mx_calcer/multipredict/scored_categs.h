#pragma once

#include "multipredict.h"

namespace NExtendedMx {
    class TScoredCategs : public IMultiPredict {
    public:
        explicit TScoredCategs(const NSc::TValue* scheme);

        TFeatureResultConst GetBestAvailableResult(const TFeatureContextDictConstProto& featContext, const TAvailVTCombinationsConst& availVtComb, TDebug& debug) const override;
    private:
        TVector<bool> GetAvailableCombinations(const TFeatureContextDictConstProto& featContext, const TAvailVTCombinationsConst& availVtComb, TDebug& debug) const;
        TFeatureResultConst MakeResultFromCombinationIndex(int selectedCateg) const;

    private:
        const TScoredCategsConstProto Scheme;
    };
} // NExtendedMx
