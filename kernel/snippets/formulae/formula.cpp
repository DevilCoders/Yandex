#include "formula.h"

#include <kernel/snippets/factors/factors.h>

#include <kernel/matrixnet/mn_sse.h>

namespace NSnippets
{
    TFormulaDescr::TFormulaDescr(const TString& name, const TFactorDomain& factors)
        : Name(name)
        , Factors(factors)
    {
    }

    size_t TFormulaDescr::GetFactorCount() const
    {
        return Factors.Size();
    }

    const char* TFormulaDescr::GetFactorName(size_t fId) const
    {
        return Factors.GetFactorInfo(fId).GetFactorName();
    }

    TString TFormulaDescr::GetName() const
    {
        return Name;
    }

    const TFactorDomain& TFormulaDescr::GetFactorDomain() const
    {
        return Factors;
    }

    TLinearFormula::TLinearFormula(const TString& name, const TFactorDomain& factors, const double* koefs)
        : TFormulaDescr(name, factors)
        , Koefs(koefs)
        {
        }

    double TLinearFormula::GetWeight(const TFactorStorage& x) const
    {
        const size_t max = Min(x.Size(), GetFactorCount());
        double res = 0.0;
        for (size_t i = 0; i < max; ++i) {
            Y_ASSERT(x[i] == x[i]);
            res += x[i] * Koefs[i];
        }
        return res;
    }

    void TLinearFormula::BatchCalc(const TFactorStorage128& f16, double (&res)[TFactorStorage128::MAX]) const
    {
        for (size_t i = 0; i < f16.GetUsed(); ++i) {
            res[i] = GetWeight(f16[i]);
        }
    }

    double TLinearFormula::GetKoef(size_t i) const
    {
        Y_ASSERT(i < GetFactorCount());
        return Koefs[i];
    }

    TMxNetFormula::TMxNetFormula(const TString& name, const TFactorDomain& factors, const NMatrixnet::TMnSseInfo& mnInfo)
        : TFormulaDescr(name, factors)
        , MnInfo(mnInfo)
    {
    }

    double TMxNetFormula::GetWeight(const TFactorStorage& fs) const
    {
        const TFactorStorage* x = &fs;
        double res = 0;
        MnInfo.DoSlicedCalcRelevs(&x, &res, 1);
        return res;
    }

    void TMxNetFormula::BatchCalc(const TFactorStorage128& f16, double (&res)[TFactorStorage128::MAX]) const
    {
        MnInfo.DoSlicedCalcRelevs(~f16, res, f16.GetUsed());
    }

}

