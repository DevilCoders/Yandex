#include "classifier_base.h"

namespace NFactClassifiers {
    float TClassifierBase::GetScore(TSimilarFact& fact, TQueryFactorStorageCache* factorStorageCache) const {
        fact.Factors = CalculateFeatures(fact, factorStorageCache);
        fact.MatrixnetValue = static_cast<float>(ClassifierModel->DoCalcRelev(fact.Factors.factors));
        return fact.MatrixnetValue;
    }
}

