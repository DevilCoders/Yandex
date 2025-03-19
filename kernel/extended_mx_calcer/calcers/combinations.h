#pragma once

#include "bundle.h"
#include <kernel/extended_mx_calcer/proto/typedefs.h>

namespace NExtendedMx {
namespace NCombinator {

    using TFeatureIdx = size_t;
    using TCombination = TVector<TFeatureIdx>;
    using TCombinations = TVector<TCombination>;
    using TCombinationIdxs = TVector<size_t>;


    struct TFeatInfo : NJsonConverters::IJsonSerializable {
    public:
        TString Name;
        bool IsBinary = false;

    public:
        TFeatInfo(const TString& name, const bool isBinary)
            : Name(name)
            , IsBinary(isBinary) {
        }

        NSc::TValue ToTValue() const override;
        void FromTValue(const NSc::TValue&, const bool validate) override;
    };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TCombinator
//    * collects available combinations
//    * intersects combinations with context
//    * fills feature vectors
//    * provide informaion about combination
//
//  common usage looks like this:
//    1) Intersect combinations with context
//    2) Fill feature vectors
//    3) Choose which feature vector is the best (client custom code)
//    4) Log combination values
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class TCombinator : public TBasedOn<TCombinationsInfoConstProto> {
    public:
        TCombinator(const NSc::TValue& scheme);

        TCombinationIdxs Intersect(const TAvailVTCombinationsConst& availCombs) const;
        NExtendedMx::TFactors FillWithFeatures(const float* features, const size_t featuresCount) const;
        NExtendedMx::TFactors FillWithFeatures(const TCombinationIdxs& combinations, const float* features, const size_t featuresCount) const;
        NSc::TValue GetCombinationValues(size_t idx) const;
        NSc::TValue GetNoShowValues() const;
        NSc::TValue ToJsonDebug() const;
        bool IsNoShowUsedInFeats() const;
        size_t GetOffsetForCombination() const;

    private:
        bool GetFeatureValueIdx(const TStringBuf& featName, const NSc::TValue& value, size_t& idx);
        void LoadAvailCombinations();
        void LoadAndEnumerateFeats();
        void LoadAvailCombinationValues();
        void EnsureValidInit() const;
        void Combination2Vector(const TCombination& comb, TVector<float>& dst) const;
        size_t FindFeatIdx(const TVector<TFeatInfo>& fi, const TStringBuf& featName) const;
        NSc::TValue GetValues(const TCombination& comb) const;

    private:
        TCombinations AllCombinations;
        TVector<TVector<TString>> AllCombinationValues;
        TVector<TFeatInfo> FeaturesOrder;
        THashMap<TString, THashMap<TString, size_t>> Feature2Value2Idx;
        THashMap<TString, TVector<TString>> Feature2Idx2Value;
        TCombination NoShowCombination;
    };

} // NCombinator
} // NExtendedMx
