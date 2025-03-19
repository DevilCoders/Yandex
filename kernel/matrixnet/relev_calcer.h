#pragma once

#include "model_info.h"

#if defined(MATRIXNET_WITHOUT_ARCADIA)
#include "without_arcadia/util/vector.h"
#include "without_arcadia/util/set.h"
#include "without_arcadia/util/types.h"
#include "without_arcadia/util/ylimits.h"
#include "without_arcadia/util/ptr.h"
#else // !defined(MATRIXNET_WITHOUT_ARCADIA)
#include <util/generic/array_ref.h>
#include <util/generic/ptr.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/ysaveload.h>
#include <kernel/factor_slices/factor_borders.h>
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
class TFactorStorage;
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

class ISharedFormulasAdapter;

namespace NMatrixnet {

class IRelevCalcer {
public:
    virtual ~IRelevCalcer() {}

    virtual double DoCalcRelev(const float* features) const = 0;

    virtual size_t GetNumFeats() const {
        return Max<size_t>();
    }

    double CalcRelev(const TVector<float> &features) const {
        return DoCalcRelev(features.data());
    }

    virtual void DoCalcRelevs(const float* const* docs_features, double* result_relev, const size_t num_docs) const {
        for (size_t i = 0; i < num_docs; ++i) {
            result_relev[i] = DoCalcRelev(docs_features[i]);
        }
    }

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    virtual void DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const;
    virtual void DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs, size_t rangeStart, size_t rangeFinish) const {
        Y_ENSURE(rangeStart == 0 && rangeFinish == Max<size_t>());
        DoSlicedCalcRelevs(features, relevs, numDocs);
    }

    void SlicedCalcRelevs(const TVector<const TFactorStorage*> &features, TVector<double> &relevs) const {
        relevs.resize(features.size());
        DoSlicedCalcRelevs(features.data(), relevs.data(), features.size());
    }

    void SlicedCalcRelevs(const TVector<TFactorStorage*> &features, TVector<double> &relevs) const {
        relevs.resize(features.size());
        DoSlicedCalcRelevs(features.data(), relevs.data(), features.size());
    }
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

    void CalcRelevs(const TVector<const float*> &docs_features, TVector<double> &result_relev) const {
        const int num_docs = docs_features.size();
        result_relev.resize(num_docs);
        DoCalcRelevs(docs_features.data(), result_relev.data(), num_docs);
    }

    void CalcRelevs(const TVector< TVector<float> > &features, TVector<double> &result_relev) const {
        TVector<const float*> fPtrs(features.ysize());
        for (int i = 0; i < features.ysize(); ++i) {
            fPtrs[i] = (features[i]).data();
        }
        CalcRelevs(fPtrs, result_relev);
    }

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    /// Special case for applying on pre-hashed categorical features, stored as possibly invalid floats
    virtual void CalcRelevsFlat(const TVector<TConstArrayRef<float>>& features, TArrayRef<double> result) const {
        TVector<const float*> fPtrs(features.ysize());
        for (int i = 0; i < features.ysize(); ++i) {
            fPtrs[i] = (features[i]).data();
        }
        DoCalcRelevs(fPtrs.data(), result.data(), features.size());
    }

    /// Set indexes of categorical features in flat feature vector.
    /// Indexes for all categorical feature from model, both used and unused (returns only used for comact model).
    virtual void CategoricalFeatureFlatIndexes(TVector<ui32>&) const {
    }
#endif

    virtual const TModelInfo* GetInfo() const {
        return nullptr;
    }

    // Get factors used in calcer
    virtual void UsedFactors(TSet<ui32>& factors) const {
        factors.clear();
    }

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    // Get factors used in calcer
    virtual void UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const {
        factors.clear();
    }

    virtual TString GetId() const;
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

    // This method is for initialization-logic which can't be in constructor.
    // Accepts pointer to a calcers fetcher.
    // The method might be called several times, so if particular implementation is supposed to be called just once,
    // it should guarantee this itself.
    //
    // Example: a calcer fetches subcalcers from fromulas' storage, but maybe subcalcers hadn't been loaded before calling calcer's constructor.
    // In this case fetching-logic should be in this method, which will be called on finalize-stage of formulas' storage.
    virtual void Initialize(const ISharedFormulasAdapter* const = nullptr) {
    }
};

using TRelevCalcerPtr = TAtomicSharedPtr<const IRelevCalcer>;

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
class IRelevCalcerLoader {
public:
    virtual ~IRelevCalcerLoader() {};

    virtual IRelevCalcer* Load(IInputStream *in) const = 0;
};

template <typename T>
class TRelevCalcerLoader : public IRelevCalcerLoader {
public:
    IRelevCalcer* Load(IInputStream *in) const override {
        THolder<T> p(new T());
        ::Load(in, *p);
        return p.Release();
    }
};
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

}   // namespace NMatrixnet
