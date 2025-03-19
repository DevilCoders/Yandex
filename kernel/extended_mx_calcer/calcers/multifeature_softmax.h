#pragma once
#include "bundle.h"

#include <library/cpp/containers/safe_vector/safe_vector.h>
#include <library/cpp/scheme/scheme.h>

#include <type_traits>

namespace NExtendedMx {
    namespace NMultiFeatureSoftmax {

        enum EFakeFeaturesMode {
            FFM_AS_INTEGER = 1,
            FFM_AS_BINARY = 1 << 1,
            FFM_AS_CATEGORICAL = 1 << 2,
        };

        using TFakeFeaturesMode = std::underlying_type<EFakeFeaturesMode>::type;

        using TCombination = TSafeVector<ui32>;

        using TCateg = int;
        const TCateg NOT_SHOWN_CATEG = -1;

        const TString WINLOSS_MC_TYPE = "multifeature_winloss_mc";

        using TRelevCalcer = NMatrixnet::IRelevCalcer;
        using TMnMcCalcer = NMatrixnet::TMnMultiCateg;
        using TRelevCalcers = TVector<const TRelevCalcer*>;

        struct TFeature {
            TString Name;
            TSafeVector<NSc::TValue> Values;
        };

        struct TMultiFeatureParams {
            TSafeVector<TCombination> Combinations;
            TSafeVector<TFeature> Features;
            TFakeFeaturesMode FakeFeaturesMode = 0u;
            TSafeVector<NSc::TValue> NoPosition;
            bool UseSourceFeaturesInUpperFml = false;
            bool NoPositionAlwaysAvailable = false;

            TMultiFeatureParams() = default;
            TMultiFeatureParams(TMultiFeatureWinLossMcInfoConstProto& bundleInfo);
            size_t GetFakeFeaturesCount() const;
            size_t GetFakeCatFeaturesCount() const;
            size_t GetFeatureCateg(size_t combIdx, size_t featIdx) const;
            TMaybe<size_t> GetCombinationIdx(const TSafeVector<NSc::TValue>& combinationFeatureValues) const;
            TMaybe<size_t> GetNoPositionCombinationIdx() const;
            const NSc::TValue& GetFeatureValue(size_t combIdx, size_t featIdx) const;
            bool HasPosFeature() const;
            size_t GetPosFeatIdx() const;
            size_t GetFeatIdx(const TStringBuf featureName) const;
            void Validate() const;
        };

        void FillSubtargetFactors(NExtendedMx::TFactors& factors, const float* blenderFactors,
                                  const size_t blenderFactorCount, const TMultiFeatureParams& mfp);
        void FillSubtargetCategoricalFactors(TCategoricalFactors& factors, const TMultiFeatureParams& mfp);
        TMaybe<size_t> GetCategIdFromIntentResult(const NSc::TValue& intentResult);

        struct TOrganicPosProbs {
            TVector<double> MainPosProbs;
            double RightProb = 0.0;
            double WizplaceProb = 0.0;
        };
    }
}
