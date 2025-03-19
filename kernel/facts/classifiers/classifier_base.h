#pragma once

#include "classifier_cache.h"
#include "classifier_data.h"
#include "similar_fact.h"

#include <kernel/facts/factors_info/factor_names.h>
#include <kernel/matrixnet/mn_dynamic.h>

namespace NFactClassifiers {
    class TClassifierBase {
    public:
        TClassifierBase(const TClassifierData& data)
            : ClassifierModel(data.ClassifierModel.Get())
        {}

        virtual ~TClassifierBase() {}

        float GetScore(TSimilarFact& fact, TQueryFactorStorageCache* factorStorageCache = nullptr) const;
        virtual NUnstructuredFeatures::TFactFactorStorage CalculateFeatures(const TSimilarFact& fact, TQueryFactorStorageCache* factorStorageCache = nullptr) const = 0;

    protected:
        const NMatrixnet::TMnSseDynamic* ClassifierModel;
    };
}
