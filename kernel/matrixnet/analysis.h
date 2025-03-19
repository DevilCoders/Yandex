#pragma once

#include <util/generic/ptr.h>
#include <util/generic/set.h>
#include <util/generic/ymath.h>
#include <util/generic/vector.h>

namespace NMatrixnet {

/*! Config structure for ExplainTrees()
 *
 * \warning Do not use Scale != 1.0 && SkipBias at the same time!
 */
struct TTreeExplanationParams {
    double Scale;
    TSet<size_t> FactorsFilter;
    bool HideDefaultValues;
    bool SkipBias;
    bool Sort;

    TTreeExplanationParams()
        : Scale(1.0)
        , HideDefaultValues(false)
        , SkipBias(false)
        , Sort(true)
    {
    }
};

/// Some information about one matrixnet tree for specific document.
struct TTreeExplanation {
    /*! Matrixnet binary feature.
     *
     * Binary feature for float factor with index Factor, which equals to
     * expression factors[Factor] < border
     */
    struct TMxBinFeature {
        size_t Factor;
        float Border;
        bool Value;
    };

    /// Tree value.
    double Value;

    /// Tree value / Sum of all tree values with the same sign * 100.
    double ValuePercent;

    /// Binary features in tree.
    TVector<TMxBinFeature> Conditions;

    /*! Sum of values for each tree with value less than Value.
     *
     * If Value < 0 then accumulate only negative tree values, else
     * accumulate only positive.
     */
    double Accum;

    /// Min of all possible current tree values.
    double Min;

    /// Max of all possible current tree values.
    double Max;

    /// Current facotrs make all conditions true in tree.
    bool DefaultValue;

    TTreeExplanation()
        : Value(0.0)
        , ValuePercent(0.0)
        , Accum(0.0)
        , Min(0.0)
        , Max(0.0)
        , DefaultValue(true)
    {
    }

    /// Default sort by Value.
    struct TPtrComparator {
        bool operator()(const TSimpleSharedPtr<TTreeExplanation>& l, const TSimpleSharedPtr<TTreeExplanation>& r) const {
            if (Y_UNLIKELY(!l || !r)) {
                return l.Get() < r.Get();
            }
            return fabs(l->Value) < fabs(r->Value);
        }
    };
};

typedef TVector<TSimpleSharedPtr<TTreeExplanation>> TTreeExplanations;


struct TFactorBorderExplanation {
    struct TBorderValue {
        float Border;
        double MnValue;

        explicit TBorderValue(const float border = 0.0
                , const double mnValue = 0.0)
            : Border(border)
            , MnValue(mnValue)
        {
        }

        bool operator <(const TBorderValue& r) const {
            return Border < r.Border;
        }
    };

    TVector<TBorderValue> Borders;
    size_t FactorId;
    float  FactorValue;

    double MnMinValue;
    double MnMaxValue;

    TFactorBorderExplanation(const size_t factorId, const float factorValue)
        : FactorId(factorId)
        , FactorValue(factorValue)
        , MnMinValue(Max<double>())
        , MnMaxValue(Min<double>())
    {
    }

    /// Comparator based on factor index.
    struct TPtrFidComparator {
        bool operator ()(const TSimpleSharedPtr<TFactorBorderExplanation>& l
            , const TSimpleSharedPtr<TFactorBorderExplanation>& r) const
        {
            if (Y_UNLIKELY(!l || !r)) {
                return l.Get() < r.Get();
            }
            return l->FactorId < r->FactorId;
        }
    };

    /// Comparator based on spread of matrixnet value for given factor.
    struct TPtrMnSpreadComparator {
        bool operator ()(const TSimpleSharedPtr<TFactorBorderExplanation>& l
            , const TSimpleSharedPtr<TFactorBorderExplanation>& r) const
        {
            if (Y_UNLIKELY(!l || !r)) {
                return l.Get() < r.Get();
            }
            const double lSpread = l->MnMaxValue - l->MnMinValue;
            const double rSpread = r->MnMaxValue - r->MnMinValue;
            return lSpread < rSpread;
        }
    };
};

typedef TVector<TSimpleSharedPtr<TFactorBorderExplanation>> TFactorBorderExplanations;

class TMnSseInfo;

struct TInfluenceCalcOptions {
    bool UseAbsoluteInfluence = false;
};

TVector<float> GetFactorInfluenceRatings(const TMnSseInfo& model,
        TConstArrayRef<float> baseline,
        TConstArrayRef<float> test,
        const TInfluenceCalcOptions& option);

struct TMnSseStatic;

TVector<TVector<double>> GetLeaves(const TMnSseStatic& mn);

struct TMnSseStaticMeta;

struct TMxBinFeature {
    size_t Factor;
    float Border;
};

TVector<TVector<TMxBinFeature>> GetFactors(const TMnSseStaticMeta& meta);

}
