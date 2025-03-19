#include "ranking_feature_pool.h"

#include <kernel/factor_slices/factor_domain.h>
#include <kernel/factors_info/one_factor_info.h>

namespace NRankingFeaturePool {
    void MakeAbsentFeatureInfo(const TString& sliceName, TFeatureInfo& res) {
        res.Clear();
        res.SetSlice(sliceName);
    }

    void MakeFeatureInfo(const TOneFactorInfo& info, TFeatureInfo& res) {
        if (Y_UNLIKELY(!info)) {
            Y_ASSERT(false);
            return;
        }
        const auto factorCountInSlice = info.GetInfo().GetFactorCount();
        if (info.GetIndex() >= factorCountInSlice) {
           const TString& sliceName = factorCountInSlice > 0 ?
               info.GetInfo().GetFactorSliceName(factorCountInSlice - 1) : "";
           MakeAbsentFeatureInfo(sliceName, res);
           return;
        }
        res.Clear();
        res.SetSlice(info.GetFactorSliceName());
        res.SetName(info.GetFactorName());
        res.SetType(FT_FLOAT);

        TVector<TString> tags;
        info.GetTagNames(tags);
        for (const auto& tag : tags) {
            res.AddTags(tag);
        }

        res.SetExtJson(ToString(info.GetExtJson()));
    }

    void MakeFeatureInfo(const IFactorsInfo& info, size_t index, TFeatureInfo& res) {
        MakeFeatureInfo(TOneFactorInfo(index, &info), res);
    }

    void MakePoolInfo(const NFactorSlices::TFactorDomain& domain, TPoolInfo& res) {
        res.Clear();
        for (auto iter = domain.Begin(); iter.Valid(); ++iter) {
            auto& elem = *res.AddFeatureInfo();

            auto info = iter.GetFactorInfo();
            if (!!info) {
                MakeFeatureInfo(info, elem);
                Y_ASSERT(ToString(iter.GetLeaf()) == info.GetFactorSliceName());
            } else {
                MakeAbsentFeatureInfo(ToString(iter.GetLeaf()), elem);
            }
        }
    }
} // NRankingFeaturePool
