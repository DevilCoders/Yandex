#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <utility>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/utility.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/ysaveload.h>

namespace NEthos {

template <typename T>
struct TIndexedFeature {
    ui64 Index;
    T Value;

    Y_SAVELOAD_DEFINE(Index, Value);

    TIndexedFeature(ui64 index = 0, T value = T())
        : Index(index)
        , Value(value)
    {
    }

    bool operator < (const TIndexedFeature& other) const {
        return Index < other.Index;
    }
};

using TIndexedFloatFeature = TIndexedFeature<float>;

using TFloatFeatureVector = TVector<TIndexedFloatFeature>;

template <typename TFeature, typename TLabel>
struct TLabeledFeatureVector {
    ui32 Index = 0;
    TVector<TFeature> Features;
    TLabel Label;
    float Weight = 1.f;

    TVector<double> ExternalFeatures;

    double OriginalTarget = 0.;

    TLabeledFeatureVector()
    {
    }

    TLabeledFeatureVector(ui32 index, const TVector<TFeature>& features, TLabel label, float weight, const double originalTarget = 0.)
        : Index(index)
        , Features(features)
        , Label(label)
        , Weight(weight)
        , OriginalTarget(originalTarget)
    {
    }

    TLabeledFeatureVector(ui32 index, TVector<TFeature>&& features, TLabel label, float weight, const double originalTarget = 0.)
        : Index(index)
        , Features(std::move(features))
        , Label(label)
        , Weight(weight)
        , OriginalTarget(originalTarget)
    {
    }

    // For consistency with TMultiLabelFeatureVector
    // so that they can be used in the same template code
    TLabel GetLabel(size_t = 0) const {
        return Label;
    }
};

template <typename TFeature, typename TLabel>
using TLabeledFeatureVectors = TVector<TLabeledFeatureVector<TFeature, TLabel>>;

enum class EBinaryClassLabel {
    BCL_UNKNOWN,
    BCL_POSITIVE,
    BCL_NEGATIVE
};

template <typename T>
struct TBinaryLabelFeatureVector: TLabeledFeatureVector<TIndexedFeature<T>, EBinaryClassLabel> {
    TBinaryLabelFeatureVector(ui32 index,
                              TVector<TIndexedFeature<T>>&& features,
                              EBinaryClassLabel label,
                              float weight,
                              const double originalTraget = 0.)
        : TLabeledFeatureVector<TIndexedFeature<T>, EBinaryClassLabel>(index,
                                                                       std::move(features),
                                                                       label,
                                                                       weight,
                                                                       originalTraget)
    {
    }

    bool IsPositive(size_t labelIndex = 0) const {
        return this->GetLabel(labelIndex) == EBinaryClassLabel::BCL_POSITIVE;
    }

    bool HasKnownMark(size_t labelIndex = 0) const {
        return this->GetLabel(labelIndex) != EBinaryClassLabel::BCL_UNKNOWN;
    }

    static TBinaryLabelFeatureVector<T> FromFeaturesString(TStringBuf featuresStr, const double threshold = 0., const size_t index = 0) {
        featuresStr.NextTok('\t'); // query id
        const double target = FromString<double>(featuresStr.NextTok('\t'));
        const EBinaryClassLabel label = target > threshold ? EBinaryClassLabel::BCL_POSITIVE : EBinaryClassLabel::BCL_NEGATIVE;

        featuresStr.NextTok('\t'); // url
        const double weight = FromString<double>(featuresStr.NextTok('\t'));

        TVector<TIndexedFeature<T> > features;
        TIndexedFeature<T> feature;
        feature.Index = 0;
        while (featuresStr) {
            feature.Value = FromString<double>(featuresStr.NextTok('\t'));
            features.push_back(feature);
            ++feature.Index;
        }

        return TBinaryLabelFeatureVector<T>(index, std::move(features), label, weight, target);
    }
};

using TBinaryLabelFloatFeatureVector = TBinaryLabelFeatureVector<float>;
class TBinaryLabelFloatFeatureVectors : public TVector<TBinaryLabelFloatFeatureVector> {
public:
    static TBinaryLabelFloatFeatureVectors FromFeatures(const TString& featuresPath, const double threhsold = 0.) {
        TBinaryLabelFloatFeatureVectors result;

        TFileInput featuresIn(featuresPath);
        TString featuresStr;
        size_t index = 0;
        while (featuresIn.ReadLine(featuresStr)) {
            result.push_back(TBinaryLabelFloatFeatureVector::FromFeaturesString(featuresStr, threhsold, index));
            ++index;
        }

        return result;
    }
};

template <typename TFeature>
struct TMultiBinaryLabelFeatureVector {
    ui32 Index = 0;
    TVector<TFeature> Features;
    TVector<ui32> PositiveLabelIndexes;
    float Weight = 1.f;

    TMultiBinaryLabelFeatureVector()
    {
    }

    TMultiBinaryLabelFeatureVector(ui32 index,
                                   const TVector<TFeature>& features,
                                   const TVector<ui32>&
                                   positiveLabelIndexes,
                                   float weight)
        : Index(index)
        , Features(features)
        , PositiveLabelIndexes(positiveLabelIndexes)
        , Weight(weight)
    {
    }

    TMultiBinaryLabelFeatureVector(ui32 index,
                                   TVector<TFeature>&& features,
                                   TVector<ui32>&& positiveLabelIndexes,
                                   const float weight)
        : Index(index)
        , Features(std::move(features))
        , PositiveLabelIndexes(std::move(positiveLabelIndexes))
        , Weight(weight)
    {
    }

    EBinaryClassLabel GetLabel(size_t labelIndex) const {
        return IsIn(PositiveLabelIndexes, labelIndex) ? EBinaryClassLabel::BCL_POSITIVE : EBinaryClassLabel::BCL_NEGATIVE;
    }

    bool IsPositive(size_t labelIndex) const {
        return IsIn(this->PositiveLabelIndexes, labelIndex);
    }

    bool HasKnownMark(size_t /* labelIndex */) const {
        return true;
    }
};

using TMultiBinaryLabelFloatFeatureVector = TMultiBinaryLabelFeatureVector<TIndexedFeature<float> >;
using TMultiBinaryLabelFloatFeatureVectors = TVector<TMultiBinaryLabelFloatFeatureVector>;

template <typename TLabel, typename TPrediction>
struct TLabelWithPrediction {
    using TSelf = TLabelWithPrediction<TLabel, TPrediction>;

    TLabel Label;
    TPrediction Prediction;

    TLabelWithPrediction(const TLabel& label, const TPrediction& prediction)
        : Label(label)
        , Prediction(prediction)
    {
    }

    inline bool operator<(const TSelf& rhs) const {
        return Prediction < rhs.Prediction;
    }
};

template <typename TPrediction>
using TBinaryLabelWithPrediction = TLabelWithPrediction<EBinaryClassLabel, TPrediction>;

using TBinaryLabelWithFloatPrediction = TBinaryLabelWithPrediction<float>;

template <typename TPrediction>
inline EBinaryClassLabel BinaryLabelFromPrediction(TPrediction prediction, TPrediction threshold) {
    if (prediction > threshold) {
        return EBinaryClassLabel::BCL_POSITIVE;
    } else {
        return EBinaryClassLabel::BCL_NEGATIVE;
    }
}

template <typename TPrediction>
using TStringLabelWithPrediction = TLabelWithPrediction<TString, TPrediction>;

using TStringLabelWithFloatPrediction = TStringLabelWithPrediction<float>;

template <typename TLabel, typename TPrediction>
class TMultiLabelPredictions {
public:
    using TSingleLabelPrediction = TLabelWithPrediction<TLabel, TPrediction>;

private:
    TVector<TSingleLabelPrediction> PositiveHeap;
    TVector<TSingleLabelPrediction> NegativeHeap;
    bool PositiveFinalized = true;
    bool NegativeFinalized = true;

public:
    TMultiLabelPredictions(const size_t sizeHint = 0)
    {
        if (sizeHint > 0) {
            Reserve(sizeHint);
        }
    }

    TMultiLabelPredictions(const TVector<TSingleLabelPrediction>& predictions) {
        for (const auto& prediction : predictions) {
            AddLabelWithPrediction(prediction);
        }
    }

    void AddLabelWithPrediction(const TSingleLabelPrediction& prediction) {
        if (prediction.Prediction > 0.) {
            PositiveFinalized = false;
            PositiveHeap.push_back(prediction);
        } else {
            NegativeFinalized = false;
            NegativeHeap.push_back(prediction);
        }
    }

    TMaybe<TSingleLabelPrediction> BestPrediction() const {
        if (PositiveHeap) {
            FinalizePositive();
            return PositiveHeap.front();
        } else if (NegativeHeap) {
            FinalizeNegative();
            return NegativeHeap.front();
        }
        return Nothing();
    }

    TMaybe<TSingleLabelPrediction> PopBestPositivePrediction() {
        FinalizePositive();

        if (PositiveHeap) {
            PopHeap(PositiveHeap.begin(), PositiveHeap.end());
            TSingleLabelPrediction result(PositiveHeap.back());
            PositiveHeap.pop_back();
            return result;
        } else {
            return Nothing();
        }
    }

    TVector<TSingleLabelPrediction> PopTopNPositivePredictions(size_t n) {
        FinalizePositive();

        n = ::Min(n, PositiveHeap.size());
        TVector<TSingleLabelPrediction> result;
        result.reserve(n);

        PopNElementsFromHeap(PositiveHeap, result, n);

        return result;
    }

    TVector<TSingleLabelPrediction> PopAllPredictions() {
        FinalizePositive();
        FinalizeNegative();

        TVector<TSingleLabelPrediction> result;
        result.reserve(PositiveHeap.size() + NegativeHeap.size());

        PopNElementsFromHeap(PositiveHeap, result, PositiveHeap.size());
        PopNElementsFromHeap(NegativeHeap, result, NegativeHeap.size());

        return result;
    }

private:
    void FinalizePositive() {
        if (Y_UNLIKELY(!PositiveFinalized)) {
            MakeHeap(PositiveHeap.begin(), PositiveHeap.end());
            PositiveFinalized = true;
        }
    }

    void FinalizeNegative() {
        if (Y_UNLIKELY(!NegativeFinalized)) {
            MakeHeap(NegativeHeap.begin(), NegativeHeap.end());
            NegativeFinalized = true;
        }
    }

    void Reserve(const size_t sizeHint) {
        PositiveHeap.reserve(sizeHint);
        NegativeHeap.reserve(sizeHint);
    }

    void PopNElementsFromHeap(TVector<TSingleLabelPrediction>& heap,
                              TVector<TSingleLabelPrediction>& result,
                              size_t count)
    {
        for (size_t i = 0; i < count; ++i) {
            PopHeap(heap.begin(), heap.end());
            result.push_back(heap.back());
            heap.pop_back();
        }
    }
};

using TMultiStringFloatPredictions = TMultiLabelPredictions<TString, float>;
using TStringLabelWithFloatPrediction = TMultiStringFloatPredictions::TSingleLabelPrediction;

struct TDenseInstance {
    ui64 QueryIdHash = 0;

    TVector<double> Features;
    double Target = 0.;
    double Weight = 1.;

    static TDenseInstance FromFeaturesString(TStringBuf featuresString) {
        const TStringBuf queryIdStr = featuresString.NextTok('\t');
        const TStringBuf targetStr = featuresString.NextTok('\t');
        const TStringBuf urlStr = featuresString.NextTok('\t');
        const TStringBuf weightStr = featuresString.NextTok('\t');

        Y_UNUSED(urlStr);

        TDenseInstance denseInstance;
        denseInstance.QueryIdHash = ComputeHash(queryIdStr);
        denseInstance.Target = FromString(targetStr);
        denseInstance.Weight = FromString(weightStr);

        while (featuresString) {
            denseInstance.Features.push_back(FromString(featuresString.NextTok('\t')));
        }

        return denseInstance;
    }
};

class TDensePool : public TVector<TDenseInstance> {
public:
    static TDensePool FromFeatures(const TString& featuresPath) {
        TDensePool pool;

        TFileInput featuresIn(featuresPath);
        TString featuresStr;
        while (featuresIn.ReadLine(featuresStr)) {
            pool.push_back(TDenseInstance::FromFeaturesString(featuresStr));
            if (pool.front().Features.size() != pool.back().Features.size()) {
                ythrow yexception() << "wrong features count on line #" << pool.size()
                                    << ": got " << pool.back().Features.size() << ", while the first line contains " << pool.front().Features.size() << "\n";
            }
        }

        return pool;
    }
};

}
