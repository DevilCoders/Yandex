#pragma once

#include <kernel/snippets/factors/factor_storage.h>

#include <util/generic/string.h>

namespace NMatrixnet {
    class TMnSseInfo;
}

namespace NSnippets
{
    extern const TFactorDomain algo2Domain;

    class TFormulaDescr
    {
        protected:
            const TString Name;
            const TFactorDomain& Factors;

        public:
            TFormulaDescr(const TString& name, const TFactorDomain& factors);
            size_t GetFactorCount() const;
            const char* GetFactorName(size_t fId) const;

            TString GetName() const;
            const TFactorDomain& GetFactorDomain() const;
    };

    class TLinearFormula : public TFormulaDescr
    {
        private:
            const double* Koefs;

        public:
            TLinearFormula(const TString& name, const TFactorDomain& factors, const double* koefs);
            double GetWeight(const TFactorStorage& x) const;
            void BatchCalc(const TFactorStorage128& f16, double (&res)[TFactorStorage128::MAX]) const;
            double GetKoef(size_t i) const;
    };

    class TMxNetFormula : public TFormulaDescr
    {
        private:
            const NMatrixnet::TMnSseInfo& MnInfo;

        public:
            TMxNetFormula(const TString& name, const TFactorDomain& factors,
                          const NMatrixnet::TMnSseInfo& mnInfo);
            double GetWeight(const TFactorStorage& fs) const;
            void BatchCalc(const TFactorStorage128& f16, double (&res)[TFactorStorage128::MAX]) const;
    };

    class TFormula : public TFormulaDescr {
    private:
        const TMxNetFormula A;
        const TLinearFormula B;

    public:
        TFormula(const TMxNetFormula& a, const TLinearFormula& b)
          : TFormulaDescr(a.GetName() + "+" + b.GetName(), a.GetFactorDomain())
          , A(a)
          , B(b)
        {
            // domain of A must be greater than domain of B
            Y_ASSERT(A.GetFactorDomain() == B.GetFactorDomain() || B.GetFactorDomain() == algo2Domain);
        }

        TFormula(const TMxNetFormula& a, const TLinearFormula& b, const TFactorDomain& redefineFactorsDomain)
          : TFormulaDescr(a.GetName() + "+" + b.GetName(), redefineFactorsDomain)
          , A(a)
          , B(b)
        {
            Y_ASSERT(GetFactorCount() >= A.GetFactorCount());
            Y_ASSERT(GetFactorCount() >= B.GetFactorCount());
        }

        double GetWeight(const TFactorStorage& x) const
        {
            return A.GetWeight(x) + B.GetWeight(x);
        }

        void BatchCalc(const TFactorStorage128& f16, double (&res)[TFactorStorage128::MAX]) const
        {
            A.BatchCalc(f16, res);
            for (size_t i = 0; i < f16.GetUsed(); ++i) {
                res[i] += B.GetWeight(f16[i]);
            }
        }
    };
}
