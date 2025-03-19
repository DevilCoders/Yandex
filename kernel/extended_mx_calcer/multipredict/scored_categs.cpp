#include "scored_categs.h"

#include <library/cpp/iterator/zip.h>
#include <kernel/extended_mx_calcer/interface/common.h>

namespace NExtendedMx {
    TScoredCategs::TScoredCategs(const NSc::TValue* scheme)
        : Scheme(scheme)
    {}

    TFeatureResultConst TScoredCategs::GetBestAvailableResult(const TFeatureContextDictConstProto& featContext, const TAvailVTCombinationsConst& availVtComb, TDebug& debug) const {
        TVector<bool> availableCombinations = GetAvailableCombinations(featContext, availVtComb, debug);
        Y_ENSURE(Scheme.Scores().Size() == availableCombinations.size());
        int bestResult = GetMaxAvailableIdxWithThreshold(Scheme.Scores(), Scheme.Scores(), availableCombinations, Scheme.ShowScoreThreshold(), Scheme.ShowScoreThreshold());
        return MakeResultFromCombinationIndex(bestResult);
    }

    TVector<bool> TScoredCategs::GetAvailableCombinations(const TFeatureContextDictConstProto& featContext, const TAvailVTCombinationsConst& availVtComb, TDebug& debug) const {
        TVector<bool> availableCombinations(Scheme.AllowedCombinations().Size(), true);
        size_t nFeatures = Scheme.Features().Size();
        size_t posFeatIdx = nFeatures;
        size_t viewTypeFeatIdx = nFeatures;
        size_t placeFeatIdx = nFeatures;
        for (size_t featIdx = 0; featIdx < nFeatures; ++featIdx) {
            const auto& featName = Scheme.Features()[featIdx].Name();
            if (featName == FEAT_POS) {
                posFeatIdx = featIdx;
            } else if (featName == FEAT_VIEWTYPE) {
                viewTypeFeatIdx = featIdx;
            } else if (featName == FEAT_PLACE) {
                placeFeatIdx = featIdx;
            }
        }

        for (size_t i = 0; i < Scheme.AllowedCombinations().Size(); ++i) {
            availableCombinations[i] = Scheme.AvailableCombinations()[i];
        }
        // ban all combinations that have a feat with value not available in featContext
        for (size_t i = 0; i < availableCombinations.size(); ++i) {
            auto allowedComb = Scheme.AllowedCombinations()[i];
            for (size_t featIdx = 0; featIdx < allowedComb.Size(); ++featIdx) {
                const auto& featName = Scheme.Features()[featIdx].Name();
                const auto& featValue = Scheme.Features()[featIdx].Values()[allowedComb[featIdx]].AsString();
                availableCombinations[i] &= FeatureValueAvailableInContext(featContext, featName, featValue, debug);
            }
        }

        // ban all combinations that have a tuple (place, viewType, pos) that is not allowed in availVtComb dict
        if (posFeatIdx != nFeatures && viewTypeFeatIdx != nFeatures) {
            for (size_t i = 0; i < availableCombinations.size(); ++i) {
                auto allowedComb = Scheme.AllowedCombinations()[i];
                ui32 pos = Scheme.Features()[posFeatIdx].Values(allowedComb[posFeatIdx]).AsPrimitive<ui32>().Get();
                TStringBuf viewType = Scheme.Features()[viewTypeFeatIdx].Values(allowedComb[viewTypeFeatIdx]).AsString();
                TStringBuf place = placeFeatIdx == nFeatures ? FEAT_PLACE_MAIN : TStringBuf(Scheme.Features()[placeFeatIdx].Values(allowedComb[placeFeatIdx]).AsString());
                availableCombinations[i] &= CombinationAvailableInContext(availVtComb, viewType, place, pos, debug);
            }
        }
        return availableCombinations;
    }

    TFeatureResultConst TScoredCategs::MakeResultFromCombinationIndex(int selectedCateg) const {
        const auto& s = Scheme;
        NSc::TValue v;
        TFeatureResultProto result(&v);
        const bool noShow = selectedCateg == -1;
        for (size_t featureIdx = 0; featureIdx < s.Features().Size(); ++featureIdx) {
            if (noShow) {
                const auto& featResult = *s.NoPosition()[featureIdx].GetRawValue();
                result[s.Features()[featureIdx].Name()].Result()->CopyFrom(featResult);
            } else {
                size_t featValueIdx = s.AllowedCombinations()[selectedCateg][featureIdx];
                const auto& featResult = *s.Features()[featureIdx].Values()[featValueIdx].GetRawValue();
                result[s.Features()[featureIdx].Name()].Result()->CopyFrom(featResult);
            }
        }
        TFeatureResultConst resultConst(v);
        return resultConst;
    }
} // NExtendedMx
