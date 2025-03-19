#pragma once

#include "dynamic_bundle.h"
#include "model_info.h"
#include "relev_calcer.h"

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#include <kernel/matrixnet/matrixnet.fbs.h>
#else
#include "without_arcadia/matrixnet.fbs.h"
#endif

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#include "analysis.h"
#include "sliced_util.h"

#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/system/defaults.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/array_ref.h>
#include <util/generic/variant.h>
#include <kernel/factor_slices/factor_borders.h>
#include <kernel/factor_slices/factor_slices.h>

#include <library/cpp/threading/thread_local/thread_local.h>
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

namespace NMatrixnet {

using TBorder = TVector<float> ;
using TBorders = TMap<ui32, TBorder>;

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
using TFactorsMap = TMap<ui32, NFactorSlices::TFullFactorIndex>;
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

/// Feature index with number of borders in matrixnet model.
struct TFeature {
    /// Feature index.
    ui32  Index = 0;

    /// Number of borders for this factor.
    ui32  Length = 0;

    TFeature() = default;

    TFeature(const ui32 index, const ui32 length)
        : Index(index)
        , Length(length)
    {
        static_assert(sizeof(TFeature) == sizeof(NMatrixnetIdl::TFeature),
                      "size TFeature and NMatrixnetIdl::TFeature should be equal");
    }

    void Swap(TFeature& obj);
    bool operator==(const TFeature& other) const {
        return Index == other.Index && Length == other.Length;
    }
    bool operator!=(const TFeature& other) const {
        return !(*this == other);
    }
#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    void Save(IOutputStream *out) const;
    void Load(IInputStream *in);
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)
};


/// FeatureSlice
struct TFeatureSlice {
    ui32 StartFeatureIndexOffset = 0;
    ui32 StartFeatureOffset = 0;
    ui32 StartValueOffset = 0;
    ui32 EndFeatureIndexOffset = 0;
    ui32 EndFeatureOffset = 0;
    ui32 EndValueOffset = 0;

    TFeatureSlice() = default;

    TFeatureSlice(
            const ui32 startFeatureIndexOffset,
            const ui32 startFeatureOffset,
            const ui32 startValueOffset,
            const ui32 endFeatureIndexOffset,
            const ui32 endFeatureOffset,
            const ui32 endValueOffset)
        : StartFeatureIndexOffset(startFeatureIndexOffset)
        , StartFeatureOffset(startFeatureOffset)
        , StartValueOffset(startValueOffset)
        , EndFeatureIndexOffset(endFeatureIndexOffset)
        , EndFeatureOffset(endFeatureOffset)
        , EndValueOffset(endValueOffset)
    {
    }
    bool operator==(const TFeatureSlice& other) const {
        return StartFeatureIndexOffset == other.StartFeatureIndexOffset &&
        StartFeatureOffset == other.StartFeatureOffset &&
        StartValueOffset == other.StartValueOffset &&
        EndFeatureIndexOffset == other.EndFeatureIndexOffset &&
        EndFeatureOffset == other.EndFeatureOffset &&
        EndValueOffset == other.EndValueOffset;
    }
    void Swap(TFeatureSlice& obj);

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    void Save(IOutputStream *out) const;
    void Load(IInputStream *in);
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)
};

// ----------------------------------------------------------------------------------------------

struct TNormAttributes {
    double DataBias = 0.0;
    double DataScale = 1.0;
    double NativeDataBias = 0.0;
    double NativeDataScale = 1.0;

    TNormAttributes() = default;

    TNormAttributes(double dataBias, double dataScale)
            : TNormAttributes(dataBias, dataScale, dataBias, dataScale)
    {}

    TNormAttributes(double dataBias, double dataScale, double nativeDataBias, double nativeDataScale)
            : DataBias(dataBias)
            , DataScale(dataScale)
            , NativeDataBias(nativeDataBias)
            , NativeDataScale(nativeDataScale)
    {}

    TNormAttributes(const TNormAttributes& o) = default;

    bool Compare(const TNormAttributes& r) const {
        return DataBias == r.DataBias &&
               DataScale == r.DataScale &&
               NativeDataBias == r.NativeDataBias &&
               NativeDataScale == r.NativeDataScale;
    }
};

const size_t MAX_COMPACT_MODEL_CLASSES = 16;

struct TValueClassLeaf {
    i32 Value = 1 << 31;
    ui8 ClassId = 0;

    TValueClassLeaf() = default;
    TValueClassLeaf(i32 value, ui8 classId)
            : Value(value)
            , ClassId(classId)
    {
        static_assert(sizeof(TValueClassLeaf) == sizeof(NMatrixnetIdl::TValueClassLeaf),
                      "sizeof TValueClassLeaf in NMatrixnetIdl:: and NMatrixnet:: must be equal");
        Y_ASSERT(classId < MAX_COMPACT_MODEL_CLASSES);
    }


    bool operator== (const TValueClassLeaf& o) const {
        return Value == o.Value && ClassId == o.ClassId;
    }
};

// ----------------------------------------------------------------------------------------------

struct TMultiData {
    struct TLeafData {
        const int* Data;
        TNormAttributes Norm;

        TLeafData() = default;

        TLeafData(const int* data, double dataBias, double dataScale)
                : Data(data), Norm(dataBias, dataScale)
        {}

        TLeafData(const int* data, double dataBias, double dataScale, double nativeDataBias, double nativeDataScale)
                : Data(data), Norm(dataBias, dataScale, nativeDataBias, nativeDataScale)
        {}
    };

    TVector<TLeafData> MultiData;
    // Total length of leaf values array, should be the same for all leaf values in model
    size_t DataSize;

    TMultiData() = default;
    TMultiData(const TVector<TLeafData>& multiData, size_t dataSize)
            : MultiData(multiData), DataSize(dataSize)
    {}
    TMultiData(size_t reserveSize, size_t dataSize)
            : MultiData(), DataSize(dataSize)
    {
        MultiData.reserve(reserveSize);
    }

    bool Compare(const TMultiData& other) const {
        return DataSize == other.DataSize &&
               MultiData.size() == other.MultiData.size() &&
               std::equal(MultiData.begin(), MultiData.end(), other.MultiData.begin(),
                          [&](const auto& l, const auto& r) {
                              return l.Norm.Compare(r.Norm) &&
                                     std::equal(l.Data, l.Data + DataSize, r.Data);
                          });
    }

    void Clear() {
        MultiData.clear();
        DataSize = 0;
    }

    bool IsClear() const {
        return DataSize == 0;
    }
};

// Compact mnmc model leaf data.  Stores (int value + classId) for each leaf instead of (n-classes)-sized vector of int values.
struct TMultiDataCompact {
    const TValueClassLeaf* Data;
    size_t DataSize;

    TNormAttributes Norm;
    size_t NumClasses;

    TMultiDataCompact(const TValueClassLeaf* leafData, size_t dataSize, const TNormAttributes& norm, size_t numClasses)
            : Data(leafData), DataSize(dataSize), Norm(norm), NumClasses(numClasses)
    {}

    TMultiDataCompact(const TVector<TValueClassLeaf>& data, const TNormAttributes& norm, size_t numClasses)
            : TMultiDataCompact(data.data(), data.size(), norm, numClasses)
    {}

    bool Compare(const TMultiDataCompact& other) const {
        return DataSize == other.DataSize &&
                NumClasses == other.NumClasses &&
                Norm.Compare(other.Norm) &&
                std::equal(Data, Data + DataSize, other.Data);
    }

    void Clear() {
        Data = nullptr;
        DataSize = 0;
        NumClasses = 0;
    }

    bool IsClear() const {
        return DataSize == 0;
    }
};

/*! Matrixnet model for sse calculations.
 *
 * Doesn't handle data, only store pointers to it. For model description see TMnSse.
 */
struct TMnSseStaticMeta {
    /// Feature border values array
    const float* Values = nullptr;
    size_t ValuesSize = 0;

    // Features can contain repeated feature indexes with different default directions
    /*! List of features, sorted by factor index
     *  Could contain repeated features with different corresponding default directions
     *  For example float feature with index 10 can be treated as -inf and as +inf for some borders,
     *  so for easier binarization we treat it as if there is 2 different features.
     */
    const TFeature* Features = nullptr;
    size_t FeaturesSize = 0;

    const TFeatureSlice *FeatureSlices = nullptr;
    size_t NumSlices = 0;

    /// If Has16Bits == true DataIndicesPtr points to ui16[] array, else it points to ui32[] array
    const void* DataIndicesPtr = nullptr;
    size_t DataIndicesSize = 0;
    bool Has16Bits = true;

    /// Default directions for nan features (NMatrixnetIdl::EFeatureDirection)
    const i8* MissedValueDirections = nullptr;
    size_t MissedValueDirectionsSize = 0;

    size_t GetIndex(size_t index) const {
        if (Has16Bits) {
            return ((ui16 *)DataIndicesPtr)[index] / 4;
        }
        return ((ui32 *)DataIndicesPtr)[index] / 4;
    }

    const int* SizeToCount = nullptr;
    size_t SizeToCountSize = 0;

    TArrayRef<TDynamicBundleComponent> DynamicBundle;

    int GetSizeToCount(size_t index) const {
        return index < SizeToCountSize ? SizeToCount[index] : 0;
    }

    TMnSseStaticMeta() = default;

    TMnSseStaticMeta(
            const float* values,      const size_t valuesSize,
            const TFeature* features, const size_t featuresSize,
            const TFeatureSlice *slices, const size_t numSlices,
            const void* dInd,         const size_t dIndSize,  bool has16Bit,
            const int* sizeToCount,   const size_t sizeToCountSize);

    TMnSseStaticMeta(
            const float* values,      const size_t valuesSize,
            const TFeature* features, const size_t featuresSize,
            const i8* missedValueDirections, const size_t missedValueDirectionsSize,
            const ui32* dInd,         const size_t dIndSize,
            const int* sizeToCount,   const size_t sizeToCountSize);

    //

    void Swap(TMnSseStaticMeta& obj);
    bool Empty() const;
    void Clear();
    bool CompareValues(const TMnSseStaticMeta& other) const;
};

struct TMnSseStaticLeaves {
    std::variant<TMultiData, TMultiDataCompact> Data;

    TMnSseStaticLeaves(const std::variant<TMultiData, TMultiDataCompact>& leaves)
            : Data(leaves)
    {}

    void Swap(TMnSseStaticLeaves& obj);
    bool Empty() const;
    void Clear();
    bool CompareValues(const TMnSseStaticLeaves& other) const;

    TNormAttributes& CommonNorm(const TString& errorMessage = "");
    const TNormAttributes& CommonNorm(const TString& errorMessage = "") const;
};

struct TMnSseStatic {
    TMnSseStaticMeta Meta;
    TMnSseStaticLeaves Leaves;

    TMnSseStatic()
            : Meta()
            , Leaves(TMultiData({}, 0))
    {}

    TMnSseStatic(
            const TMnSseStaticMeta& features,
            const int* data,          const size_t dataSize,
            const double bias,        const double scale);

    TMnSseStatic(
            const TMnSseStaticMeta& features,
            const TMnSseStaticLeaves& leaves);

    void Swap(TMnSseStatic& obj);
    bool Empty() const;
    void Clear();
    bool CompareValues(const TMnSseStatic& other) const;
};

static const ui32 FLATBUFFERS_MN_MODEL_MARKER = (ui32)-1;
static const ui32 FLATBUFF_MODEL_MODEL_HEADER_SIZE = 8;

/*! SSE-optimized matrixnet calcer for multiple documents.
 *
 * It is recommend to calculate model value for multiple documents at one call instead
 * of calculating value separate for each document.
 */

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
namespace NDetail {
    struct TCalcContext {
        TVector<const float*> FinalFeatures;
        TVector<float> ExtFeaturesHolder;
        TVector<ui8> MxNet128Buffer;
    };
}
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

class TMnSseInfo: public IRelevCalcer {
public:
    TMnSseInfo();

    /*! Constructor for initializing from static data.
     *
     * Warning! This constructor doesn't copy any data.
     */
    TMnSseInfo(const void* const buf, const size_t size, const char* const name = nullptr);

    /*! Constructor for initializing from static data.
     *
     * Warning! This constructor doesn't copy any data.
     */
    TMnSseInfo(
            const TMnSseStaticMeta& features,
            const int* data,          const size_t dataSize,
            const double bias,        const double scale);

    void InitStatic(const void* const buf, const size_t size, const char* const name = nullptr);

    void Swap(TMnSseInfo& obj);

    /// Check if model is empty.
    bool Empty() const;

    /// Removes all internal data from model so that the model become empty.
    virtual void Clear();

    /// Total number of features, used in model
    int NumFeatures() const;

    /// Total number of binary features (after binarization), used in model
    ui64 NumBinFeatures() const;

    /// Number of trees in model
    int NumTrees() const;

    /// Number of trees with specific size in model
    int NumTrees(const int treeSize) const;

    /// Maximum size of trees in model
    int MaxTreeSize() const;

    /// Model bias.
    /// \warning Valid only for single value models
    double Bias() const;

    /// Model scale.
    /// \warning Valid only for single value models
    double Scale() const;

    /// Custom model bias, caused by renorm.
    /// \warning Valid only for single value models
    double NativeBias() const;

    /// Custom model scale, caused by renorm.
    /// \warning Valid only for single value models
    double NativeScale() const;

    /// Get factors used in matrixnet
    void UsedFactors(TSet<ui32>& factors) const override;

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    /// Get factors used in matrixnet
    void UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const override;
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

    /// Get maximum factor index
    ui32 MaxFactorIndex() const;

    const TMnSseStatic& GetSseDataPtrs() const;

    /*! Update normalization coefficients.
     *
     * Expecting scale=max-min and bias=-min of all possible valus
     * on gived data set. So new s' /= scale and b' = (b + bias) / scale
     */
    void UpdateNormalization(const double bias, const double scale);

    /// Renorm matrixnet
    void Renorm(const double scale, const double bias);

    /// Set Scale and Bias
    void SetScaleAndBias(const double scale, const double bias);

    /// Set native Scale and Bias
    void SetNativeScaleAndBias(const double scale, const double bias);

    /// Update native Scale and Bias
    void UpdateNativeScaleAndBias();

    /// Matrixnet borders
    void Borders(TBorders& borders) const;

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    /// Matrixnet factors mapping
    void FactorsMap(TFactorsMap& factorsMap) const;

    /*! Explain matrixnet trees.
     *
     * For each mn tree find active branch, it's value and calculate some statistics.
     * Result is sorted by absolute tree value in desc order.
     * \warning You are responsible for freeing all data in expls array.
     */
    void ExplainTrees(const float* factors, const TTreeExplanationParams& cfg, TTreeExplanations &expl) const;

    /// Explain factors usage in matrixnet model.
    void ExplainFactorBorders(const float* factors, TFactorBorderExplanations &expls) const;

    /*! Explain factors usage in matrixnet model (only for specific factors).
     *
     * \param factorIds explain only factors in this set.
     * For each factors check matrixnet value for each border.
     * Result is sorted by spread of mn value in desc order.
     * \warning You are responsible for freeing all data in expls array.
     */
    void ExplainFactorBorders(const float* factors, const TSet<size_t> &factorIds, TFactorBorderExplanations &expls) const;

    const TString& MD5() const;

    const TVector<TFeatureSlice>& GetSlices() const {
        return Slices;
    }
    const TVector<TString>& GetSliceNames() const {
        return SliceNames;
    }

    /// Check if model has "CustomSlices" flag
    /// If true, model can use arbitrary names of the the slices.
    /// It is not guaranteed that TFactorStorage can supply factors for such slices.
    bool HasCustomSlices() const;

    template <class TFactorStorageType>
    void CustomSlicedCalcRelevs(const TFactorStorageType features[], TVector<double>& relevs, const size_t numDocs) const;
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

public:
    // from IRelevCalcer
    /*! Returns MaxFactorIdx + 1
     **/
    size_t GetNumFeats() const override {
        return MaxFactorIdx + 1;
    }
    double DoCalcRelev(const float* factors) const override;
    void DoCalcRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs) const override;
    virtual void DoCalcRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs, const size_t numSlices) const;
    void DoCalcMultiDataRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs, const size_t numSlices, const size_t numValues) const;
    using IRelevCalcer::CalcRelevs;

    class TPreparedBatch;
    struct TPreparedBatchDeleter { static void Destroy(TPreparedBatch*); };
    using TPreparedBatchPtr = THolder<TPreparedBatch, TPreparedBatchDeleter>;
    void DoCalcRelevs(const TPreparedBatch& preparedBatch, double* resultRelev) const;
    virtual void DoCalcRelevs(const TPreparedBatch& preparedBatch, double* resultRelev, const size_t rangeBegin, const size_t rangeFinish) const;
    void DoCalcMultiDataRelevs(const TPreparedBatch& preparedBatch, double* resultRelev, const size_t numValues) const;
    TPreparedBatchPtr CalcBinarization(const float* const* docsFactors, const size_t numDocs, const size_t numSlices) const;
    void CalcRelevs(const TPreparedBatch& preparedBatch, TVector<double> &result_relev) const;
    TPreparedBatchPtr CalcBinarization(const TVector<const float*> &docs_features) const;
    TPreparedBatchPtr CalcBinarization(const TVector< TVector<float> > &features) const;

private:
    void DoCalcMultiDataRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs, const size_t numSlices, const size_t numValues, const size_t rangeBegin, const size_t rangeFinish) const;
    virtual void DoCalcRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs, const size_t numSlices, const size_t rangeBegin, const size_t rangeFinish) const;

    void DoCalcMultiDataRelevs(const TPreparedBatch& preparedBatch, double* resultRelev, const size_t numValues, const size_t rangeBegin, const size_t rangeFinish) const;

public:
#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    void DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const override;
    void DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs, size_t rangeBegin, size_t rangeFinish) const override;

    struct TSlicedPreparedBatch {
        TPreparedBatchPtr PreparedBatch;
        const TFactorStorage* const* Features = nullptr;

        size_t GetDocCount() const;
    };

    TSlicedPreparedBatch DoSlicedCalcBinarization(const TFactorStorage* const* features, size_t numDocs) const;
    void DoSlicedCalcRelevs(const TSlicedPreparedBatch& features, double* relevs) const;

    TSlicedPreparedBatch SlicedCalcBinarization(const TVector<const TFactorStorage*> &features) const;
    TSlicedPreparedBatch SlicedCalcBinarization(const TVector<TFactorStorage*> &features) const;
    using IRelevCalcer::SlicedCalcRelevs;
    void SlicedCalcRelevs(const TSlicedPreparedBatch& slicedPreparedBatch, TVector<double> &relevs) const;
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

    const TModelInfo* GetInfo() const override {
        return &Info;
    }

    void SetSlicesFromInfo();
    void SetDynamicBundleFromInfo();

    void SetInfo(const TString& key, const TString& value) {
        Info[key] = value;
    }
protected:
    void InitMaxFactorIndex();

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
private:
    static NThreading::TThreadLocalValue<NDetail::TCalcContext> CalcContext;

    /// Parse slice names from sliceString
    /// Skips name validation if allowCustomNames is true
    void InitSlices(const TString& sliceString, bool allowCustomNames);
    size_t SlicedCalcNumberOfFeaturesPerDoc() const;
    ui32 ComputeFeatureOffset(const ui32 featureIndex) const;
    ui32 ComputeValueOffset(const ui32 featureIndex) const;

    template <class TFactorStorageType>
    void FillSlicedFeatures(
            const TFactorStorageType features[],
            const size_t numDocs,
            TVector<const float*>& finalFeatures,
            TVector<float>& extFeaturesHolder) const;
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

public:
    TModelInfo Info;

protected:
    TMnSseStatic Matrixnet;
    const void* SourceData = nullptr;
    size_t SourceSize = 0;
    // computed lazily in MaxFactorIndex()
    size_t MaxFactorIdx = size_t(-1);
    TMaybe<TDynamicBundle> DynamicBundle;

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
    TVector<TFeatureSlice> Slices;
    TVector<TString> SliceNames;
    // computed lazily in MD5()
    mutable TString MD5Sum;
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)
};

typedef TAtomicSharedPtr<const TMnSseInfo> TMnSsePtr;

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
TString ModelNameFromFilePath(const TStringBuf fname);

template <class TFactorStorageType>
void TMnSseInfo::FillSlicedFeatures(
        const TFactorStorageType features[],
        const size_t numDocs,
        TVector<const float*>& finalFeatures,
        TVector<float>& extFeaturesHolder) const
{
    finalFeatures.resize(numDocs * Slices.size());
    const size_t numFeatsPerDoc = SlicedCalcNumberOfFeaturesPerDoc();
    extFeaturesHolder.clear();
    extFeaturesHolder.reserve(numFeatsPerDoc * numDocs);

    for (size_t i = 0; i < SliceNames.size(); ++i) {
        const auto& sliceName = SliceNames[i];
        const size_t sliceFeaturesLength = Slices[i].EndFeatureIndexOffset - Slices[i].StartFeatureIndexOffset;
        for (size_t j = 0; j < numDocs; ++j) {
            auto curFeatures = GetFactorsRegion(features[j], sliceName);
            if (curFeatures.size() > 0) {
                finalFeatures[j + i * numDocs] = GetOrCopyFeatures(curFeatures, sliceFeaturesLength, extFeaturesHolder);
                continue;
            }

            finalFeatures[j + i * numDocs] = CreateZeroes(sliceFeaturesLength, extFeaturesHolder);
        }
    }
}

template <class TFactorStorageType>
void TMnSseInfo::CustomSlicedCalcRelevs(const TFactorStorageType features[], TVector<double>& relevs, const size_t numDocs) const {
    Y_ENSURE(!this->GetSlices().empty());
    TVector<const float*> finalFeatures;
    TVector<float> extFeaturesHolder;
    relevs.resize(numDocs);
    FillSlicedFeatures<TFactorStorageType>(features, numDocs, finalFeatures, extFeaturesHolder);
    DoCalcRelevs(finalFeatures.data(), relevs.data(), numDocs, GetSlices().size());
}
#endif // !defined(MATRIXNET_WITHOUT_ARCADIA)

}
