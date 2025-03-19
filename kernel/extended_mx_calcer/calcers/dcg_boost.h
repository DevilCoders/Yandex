#pragma once

#include "multifeature_softmax.h"

void CalcCategRelevBoosts(
        THashMap<NExtendedMx::NMultiFeatureSoftmax::TCateg, double>& categRelevDCGBoosts,
        NExtendedMx::TDebug& dbgLog,
        const double wizRelev,
        const TVector<double>& docRelevs,
        size_t& bestPosByRelevance,
        const size_t dcgDocCount,
        const THashMap<NExtendedMx::NMultiFeatureSoftmax::TCateg, NExtendedMx::NMultiFeatureSoftmax::TOrganicPosProbs>& organicPosProbs,
        const NExtendedMx::NMultiFeatureSoftmax::TMultiFeatureParams& multiFeatureParams,
        const TVector<NExtendedMx::NMultiFeatureSoftmax::TCateg>& upperCategs,
        const int lossCategShift,
        const THashMap<size_t, std::pair<double, double>>& viewTypeShift,
        const size_t viewTypeFeatureIndex
);
