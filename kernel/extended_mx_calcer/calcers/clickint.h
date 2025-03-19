#pragma once

#include "bundle.h"
#include "random.h"

#include <kernel/extended_mx_calcer/proto/typedefs.h>
#include <kernel/extended_mx_calcer/interface/extended_relev_calcer.h>

namespace NExtendedMx {

    template <typename TCont>
    using TValidFeats = THashMap<TString, TCont>;
    using TValidFeatsSet = TValidFeats<TSet<TString>>;
    using TValidFeatsVec = TValidFeats<TVector<TString>>;

    static const size_t NO_STEP = Max<size_t>();

    void ExtractAdditionalFeatsResult(const TExtendedRelevCalcer* calcer, TAdditionalFeaturesConstProto additionalFeatures, TCalcContext& ctx, const TVector<float>& row, bool hasStep);

    template <typename TCont>
    void IntersectFeats(TAdditionalFeaturesConstProto additionalFeatures, TCalcContextMetaProto ctxMeta, TValidFeats<TCont>& validFeats);
    void FillStepAndInsertFeatsWithOffset(size_t stepCount, size_t totalRows, size_t offset, const float* fPtr, const size_t fSize, TFactors& dst);
    void ReplaceRepeatadly(TFactors& orig, const TFactors& patch, size_t inRowOffset, size_t rowsPerFeat);

    void FillPredefinedFeatures(const TExtendedRelevCalcer* calcer, TFactors& factors, const float* fPtr, const size_t fSize, size_t stepCount,
                                TAdditionalFeaturesConstProto additionalFeatures, TCalcContext& context);

    void RandomSelectAdditionalFeatures(const TExtendedRelevCalcer& calcer, const TAdditionalFeaturesConstProto& additionalFeats, TRandom& random, TCalcContext& context);
} // NExtendedMx
