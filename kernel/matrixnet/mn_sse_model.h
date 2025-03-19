#pragma once

#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/system/defaults.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>

#include "mn_sse.h"
#include "relev_calcer.h"

namespace NMatrixnet {
/*! Matrixnet in format optimized for SSE calculations on multiple documents.
 *
 * It is recommend to calculate model value for multiply documents at one call instead
 * of calculating value separate for each document.
 */
class TMnSseModel: public TMnSseInfo {
public:
    using TOwnedMultiDataVector = TVector<TVector<int>>;
    using TOwnedMultiDataCompact = TVector<TValueClassLeaf>;
    using TOwnedMultiData = std::variant<TOwnedMultiDataVector, TOwnedMultiDataCompact>;

    TMnSseModel();


    /*! Constructor for initializing from dynamic model.
     *
     * Warning! This constructor copy all data.
     */
    TMnSseModel(const TMnSseModel& matrixnet);

    /*! Constructor for initializing from static data.
     *
     * Warning! This constructor copy all data.
     */
    TMnSseModel(const TMnSseStatic& matrixnet);

    /*! Constructor for initializing from static data.
     *
     * Warning! This constructor copy all data.
     */
    TMnSseModel(const TMnSseInfo& matrixnet);

    TMnSseModel& operator=(const TMnSseModel& matrixnet);

    void CopyFrom(const TMnSseStatic& matrixnet);
    void CopyFrom(const TMnSseInfo& matrixnet);
    void CopyFrom(const TMnSseStaticMeta& features, const TMnSseStaticLeaves& leaves);

    void Swap(TMnSseModel& obj);

    /// Removes all internal data from model so that the model become empty.
    void Clear() override;

    virtual void Save(IOutputStream *out) const;

    virtual void Load(IInputStream *in);

    /// Remove from model all unused features and factors.
    void RemoveUnusedFeatures();

    /*! Remove last trees to leave only num trees in model.
     *
     * \warning This method doesn't guarantee the removal of last iterations trees,
     *          it just remove trees from the end of forest.
     * \warning This method doesn't remove unused features after truncating.
     *          To do this call RemoveUnusedFeatures.
     */
    void Truncate(const int num);

    void RemapFactors(const TMap<ui32,ui32> &remap, bool strictCheck = false);

    TMnSseStatic GetSseDataPtrs() const;

protected:
    /// Update pointers to new data
    void UpdateMatrixnet();
private:
    /// Old format deserializer/serializer
    void LoadOld(IInputStream* in);
    // Flatbuf deserializer/serializer
    void LoadFromFlatbuf(IInputStream* in);
    void SaveFlatbuf(IOutputStream* out) const;
protected:
    /*! List of features (factor borders).
     *
     *  Borders for the same factor grouped together. Groups sorted the
     *  same as Features.
     */
    TVector<float> Values;

    /*! List of factors, sorted by factor index
     *  Could contain repeated features with different corresponding default directions
     *  For example float feature with index 10 can be treated as -inf and as +inf for some borders,
     *  so for easier binarization we treat it as if there is 2 different features.
     */
    TVector<TFeature> Features;

    /// Possibly empty list of default directions (NMatrixnetIdl::EFeatureDirection) for nan features
    TVector<i8> MissedValueDirections;

    /*! Condition indices, scaled by 4.
     *
     * Conditions from the same tree grouped together. Trees (condition
     * grous) sorted by number of conditions. To get number of trees
     * with the same number of conditions use SizeToCount[numberOfCond]
     *
     * \warning All indices scaled by factor 4.
     */
    TVector<ui32>  DataIndices;

    /// For index I store number of trees with I conditions in DataIndices.
    TVector<int> SizeToCount;

    /*! List of trees values in the same order as trees appears in DataIndices.
     *
     * For each tree store 2^numberOfCond values for each possible conditions
     * values, grouped together.
     *
     * \warning In fact, all data is shifted by constant (1 << 31), because of
     * troubles with SSE signed convertion i32 -> i64.
     */
    TOwnedMultiData MultiData;
};

typedef TAtomicSharedPtr<const TMnSseModel> TMnSseModelPtr;

}
