#pragma once

#include "model_info.h"
#include "mn_dynamic.h"

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/array_ref.h>

namespace NMatrixnet {

/*! Multi-category matrixnet model.
 *
 * Consist of separate matrixnet models for each class.
 */

class TMnMultiCategImpl;

class TMnMultiCateg: public virtual IRelevCalcer {
public:
    TMnMultiCateg();
    TMnMultiCateg(const void* const buf, const size_t size, const char* const name = nullptr);
    TMnMultiCateg(const TMnMultiCateg& other);
    TMnMultiCateg(TVector<TMnSseDynamicPtr>&& models, TVector<double>&& classValues);

    ~TMnMultiCateg();

    void Swap(TMnMultiCateg &obj);

    TConstArrayRef<double> CategValues() const;

    virtual void Save(IOutputStream *out) const;
    virtual void Load(IInputStream *in);

    /** Calculate values for all possible categories for each document in terms of probabilities.
     *
     * docs_factors -- vector where each element is a document (factors plane)
     * result_categ_values -- 2-dimensional array with size docs_factors.size() * CategValues().size()
     *          i*CategValues().size() + j is value of j-th category for i-th document
     */
    void CalcCategs(const TVector< TVector<float> > &docsFactors, double* resultCategValues) const;

    /** Calculate values for all possible categories for each document (ranks are not normilized).
     *
     * docs_factors -- vector where each element is a document (factors plane)
     * result_categ_values -- 2-dimensional array with size docs_factors.size() * CategValues().size()
     *          i*CategValues().size() + j is value of j-th category for i-th document
     */
    void CalcCategoriesRanking(const TVector< TVector<float> > &docsFactors, double* resultCategValues) const;

    /// Get factors used in each matrixnet
    void UsedFactors(TSet<ui32>& factors) const override;
    void UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const override;

    /// Total number of features, used in model
    int NumFeatures() const;

    const TString& MD5() const;

public:
    TMnMultiCateg(THolder<TMnMultiCategImpl> impl);

    size_t GetNumFeats() const override;
    double DoCalcRelev(const float* factors) const override;
    void DoCalcRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs) const override;
    void SetInfo(const TString& key, const TString& value);
    const TModelInfo* GetInfo() const override;
private:
    THolder<TMnMultiCategImpl> Implementation;
};

typedef TAtomicSharedPtr<const TMnMultiCateg> TMnMultiCategPtr;

THolder<TMnMultiCateg> MakeCompactMultiCateg(TVector<TMnSseDynamicPtr>&& models, TVector<double>&& classValues);

static const ui32 FLATBUFFERS_MNMC_MODEL_MARKER = (ui32)-2;

}
