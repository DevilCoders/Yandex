#include "relev_fml.h"

#include <util/generic/singleton.h>

float TPolynomDescrStatic::Calc(const float* factors) const {
    if (Fml) {
        return Fml(factors);
    } else if (Descr) {
        return Descr->Calc(factors);
    }
    return 0.0;
}

void TPolynomDescrStatic::MultiCalc(const float* const* factors, float *res, const size_t size) const {
    if (FmlSSE) {
        size_t i = 0;
        for (; i + 4 <= size; i += 4) {
            FmlSSE(factors + i, res + i);
        }
        if (i < size) {
            const float *rest[4];
            float restRes[4];
            memcpy(rest, factors + i, (size - i) * sizeof(float*));
            for (size_t j = size - i; j < 4; ++j) {
                rest[j] = rest[0];
            }
            FmlSSE(rest, restRes);
            memcpy(res + i, restRes, (size - i) * sizeof(float));
        }
    } else if (Fml) {
        for (size_t i = 0; i < size; ++i) {
            res[i] = Fml(factors[i]);
        }
    } else if (Descr) {
        for (size_t i = 0; i < size; ++i) {
            res[i] = Descr->Calc(factors[i]);
        }
    } else {
        for (size_t i = 0; i < size; ++i) {
            res[i] = 0.0;
        }
    }
}

void TPolynomDescrStatic::MultiCalc(const TVector< TVector<float> > &factors, TVector<float> res) const {
    Y_ASSERT(factors.ysize() == res.ysize());
    const size_t size = Min(factors.size(), res.size());
    TVector<const float*> factorPtrs;
    factorPtrs.resize(factors.ysize());
    for (int i=0; i<factors.ysize(); ++i) {
        factorPtrs[i] = &*(factors[i].begin());
    }
    MultiCalc(&*factorPtrs.begin(), &*res.begin(), size);
}

//////////////////////////////////////////////////////////////////////////////

void TRelevanceFormulaRegistry::Add(const TPolynomDescrStatic* fml) {
    const char* name = fml->Name;
    if (Storage.contains(name))
        ythrow yexception() << "Duplicate formula name: " << name;
    Storage[name] = fml;
}

const TPolynomDescrStatic* TRelevanceFormulaRegistry::Find(const char* name) const {
    TStorage::const_iterator it = Storage.find(name);
    return it == Storage.end() ? nullptr : it->second;
}

void TRelevanceFormulaRegistry::GetAllNames(TVector<TString>* result) const {
    result->clear();
    for (TStorage::const_iterator it = Storage.begin(); it != Storage.end(); ++it)
        result->push_back(it->first);
}

TRelevanceFormulaRegistry* RelevanceFormulaRegistry() {
    return Singleton<TRelevanceFormulaRegistry>();
}

double RelevFmlCalcFactor(const SRelevanceFormula &fml
    , const float *factorVals
    , int factorId)
{
    TVector< TVector<int> > params;
    TVector<float> weights;
    fml.GetFormula(&params, &weights);

    double totalCoeff = .0;

    for (size_t i = 0; i < params.size(); ++i) {
        TVector<int> &factors = params[i];
        double coeff = weights[i];
        bool foundNet = false;
        for (size_t j = 0; j < factors.size(); ++j) {
            if (factors[j] == factorId)
                foundNet = true;
            else
                coeff *= factorVals[factors[j]];
            if (foundNet)
                totalCoeff += coeff;
        }
    }

    return totalCoeff;
}

