#pragma once

#include <kernel/text_machine/parts/common/types.h>
#include <kernel/text_machine/parts/common/seq4.h>

#include <util/generic/utility.h>
#include <util/generic/ymath.h>
#include <util/generic/ylimits.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>

namespace NTextMachine {
namespace NCore {
namespace NAggregator {
    // Buffers
    //
    class TValuesBuf {
    private:
        size_t Len = 0;

    public:
        void Init(size_t len) {
            Len = len;
        }

        size_t Size() const {
            return Len;
        }
    };

    template <typename TBase>
    class TFeaturesMixin : public TBase {
    private:
        float* Features = nullptr;

    public:
        void SetFeatures(float* features) {
            Y_ASSERT(!Features);
            Features = features;
            Y_ASSERT(Features);
        }

        void FillFeatures(float value = 0.0) {
            Fill(Features, Features + TBase::Size(), value);
        }

        float GetFeature(size_t index) const {
            Y_ASSERT(index < TBase::Size());
            return Features[index];
        }

        float& Feature(size_t index) {
            Y_ASSERT(index < TBase::Size());
            return Features[index];
        }

        NSeq4f::TSeq4f GetFeaturesSeq() const {
            return NSeq4f::TSeq4f(Features, TBase::Size());
        }

        template <typename TSeq>
        void StoreFeaturesSeq(TSeq&& seq) {
            NSeq4f::Copy(std::forward<TSeq>(seq), Features);
        }
    };

    template <typename TBase>
    class TSingleWeightMixin : public TBase {
    private:
        float OneWeight = 0.0f;

    public:
        void SetWeight(float weight)
        {
            OneWeight = weight;
        }

        float GetWeight() const {
            return OneWeight;
        }

        float GetWeight(size_t /*index*/) const { // To be compatible with TWeightsMixin
            return OneWeight;
        }

        float& Weight() {
            return OneWeight;
        }

        NSeq4f::TConstSeq4f GetWeightsSeq() const {
            return NSeq4f::TConstSeq4f(OneWeight, TBase::Size());
        }
    };

    template <typename TBase>
    class TWeightsMixin : public TBase {
    private:
        float* Weights = nullptr;

    public:
        void SetWeights(float* weights) {
            Y_ASSERT(!Weights);
            Weights = weights;
            Y_ASSERT(Weights);
        }

        void FillWeights(float value = 1.0) {
            Fill(Weights, Weights + TBase::Size(), value);
        }

        float GetWeight(size_t index) const {
            Y_ASSERT(index < TBase::Size());
            return Weights[index];
        }

        float& Weight(size_t index) {
            Y_ASSERT(index < TBase::Size());
            return Weights[index];
        }

        NSeq4f::TSeq4f GetWeightsSeq() const {
            return NSeq4f::TSeq4f(Weights, TBase::Size());
        }

        template <typename TSeq>
        void StoreWeightsSeq(TSeq&& seq) {
            NSeq4f::Copy(std::forward<TSeq>(seq), Weights);
        }
    };

    using TFeaturesBuf = TFeaturesMixin<TValuesBuf>;
    using TSingleWeightFeaturesBuf = TSingleWeightMixin<TFeaturesBuf>;
    using TSingleWeightBuf = TSingleWeightMixin<TValuesBuf>;
    using TWeightsFeaturesBuf = TWeightsMixin<TFeaturesBuf>;
    using TWeightsBuf = TWeightsMixin<TValuesBuf>;

    // Weights access, for abstract operations
    //
    struct TWeightWriteAccessor {
        template <typename TBuf>
        static void Init(TBuf& buf, float* data) {
            buf.SetWeights(data);
        }

        template <typename TBuf>
        static void Fill(TBuf& buf, float value) {
            buf.FillWeights(value);
        }

        template <typename TBuf, typename TSeq>
        static void Store(TBuf& buf, TSeq&& seq) {
            buf.StoreWeightsSeq(std::forward<TSeq>(seq));
        }
    };

    struct TWeightReadAccessor {
        template <typename TBuf>
        static float Get(const TBuf& buf, size_t i) {
            return buf.GetWeight(i);
        }

        template <typename TBuf>
        static auto Seq(const TBuf& buf)
            -> decltype(buf.GetWeightsSeq())
        {
            return buf.GetWeightsSeq();
        }
    };

    struct TWeightAccessor
        : public TWeightReadAccessor
        , public TWeightWriteAccessor {};

    // Features access, for abstract operations
    //
    struct TFeatureWriteAccessor {
        template <typename TBuf>
        static void Init(TBuf& buf, float* data) {
            buf.SetFeatures(data);
        }

        template <typename TBuf>
        static void Fill(TBuf& buf, float value) {
            buf.FillFeatures(value);
        }

        template <typename TBuf, typename TSeq>
        static void Store(TBuf& buf, TSeq&& seq) {
            buf.StoreFeaturesSeq(std::forward<TSeq>(seq));
        }
    };

    struct TFeatureReadAccessor {
        template <typename TBuf>
        static float Get(const TBuf& buf, size_t i) {
            return buf.GetFeature(i);
        }

        template <typename TBuf>
        static auto Seq(const TBuf& buf)
            -> decltype(buf.GetFeaturesSeq())
        {
            return buf.GetFeaturesSeq();
        }
    };

    struct TFeatureAccessor
        : public TFeatureReadAccessor
        , public TFeatureWriteAccessor {};

    // Computing accessors
    //
    struct TProductAccessor {
        template <typename TBuf>
        static float Get(const TBuf& buf, size_t i) {
            return buf.GetFeature(i) * buf.GetWeight(i);
        }

        template <typename TBuf>
        static auto Seq(const TBuf& buf)
            -> decltype(NSeq4f::Mul(buf.GetFeaturesSeq(), buf.GetWeightsSeq())) {
            return NSeq4f::Mul(buf.GetFeaturesSeq(), buf.GetWeightsSeq());
        }
    };

    template <size_t Skew=2>
    struct TSkewedProductAccessor {
        template <typename TBuf>
        static float Get(const TBuf& buf, size_t i) {
            return buf.GetFeature(i) * pow(buf.GetWeight(i), static_cast<float>(Skew));
        }

        template <typename TBuf>
        static auto Seq(const TBuf& buf)
            -> decltype(NSeq4f::Mul(buf.GetFeaturesSeq(), NSeq4f::Pow(buf.GetWeightsSeq(), Skew)))
        {
            return NSeq4f::Mul(buf.GetFeaturesSeq(), NSeq4f::Pow(buf.GetWeightsSeq(), Skew));
        }
    };

    // Abstract operations
    //
    template <typename TAccX, typename TAccY, int InitValue>
    struct TOpBase {
        using TAccessorX = TAccX;
        using TAccessorY = TAccY;

        template <typename TBufX>
        static void Init(TBufX& buf, float* data) {
            TAccessorX::Init(buf, data);
        }

        template <typename TBufX>
        static void Fill(TBufX& buf) {
            TAccessorX::Fill(buf, InitValue);
        }

        template <typename TBufX, typename TSeq>
        static void Store(TBufX& buf, TSeq&& seq) {
            TAccessorX::Store(buf, std::forward<TSeq>(seq));
        }

        template <typename TBufX, typename TBufY>
        static void Store(TBufX& bufX, const TBufY& bufY) {
            TAccessorX::Store(bufX, TAccessorY::Seq(bufY));
        }
    };

    template <typename TAccessorX, typename TAccessorY>
    struct TAddOp : public TOpBase<TAccessorX, TAccessorY, 0> {
        template <typename TBufX, typename TBufY>
        auto operator() (const TBufX& lhs, const TBufY& rhs) const
            -> decltype(NSeq4f::Add(TAccessorX::Seq(lhs), TAccessorY::Seq(rhs)))
        {
            return NSeq4f::Add(TAccessorX::Seq(lhs), TAccessorY::Seq(rhs));
        }
    };

    template <typename TAccessorX, typename TAccessorY>
    struct TMinOp : public TOpBase<TAccessorX, TAccessorY, 1> {
        template <typename TBufX, typename TBufY>
        auto operator() (const TBufX& lhs, const TBufY& rhs) const
            -> decltype(NSeq4f::Min(TAccessorX::Seq(lhs), TAccessorY::Seq(rhs)))
        {
            return NSeq4f::Min(TAccessorX::Seq(lhs), TAccessorY::Seq(rhs));
        }
    };

    template <typename TAccessorX, typename TAccessorY>
    struct TMaxOp : public TOpBase<TAccessorX, TAccessorY, 0> {
        template <typename TBufX, typename TBufY>
        auto operator() (const TBufX& lhs, const TBufY& rhs) const
            -> decltype(NSeq4f::Max(TAccessorX::Seq(lhs), TAccessorY::Seq(rhs)))
        {
            return NSeq4f::Max(TAccessorX::Seq(lhs), TAccessorY::Seq(rhs));
        }
    };

    template <typename TAccessorX, typename TAccessorY>
    struct TNormalizeOp {
        template <typename TBufX, typename TBufY>
        auto operator() (const TBufX& lhs, const TBufY& rhs) const
            -> decltype(NSeq4f::Min(
                NSeq4f::Div(
                    TAccessorX::Seq(lhs),
                    NSeq4f::Max(TAccessorY::Seq(rhs), NSeq4f::TConstSeq4f(FloatEpsilon, rhs.Size()))),
                NSeq4f::TConstSeq4f(1.0f, rhs.Size())))
        {
            return NSeq4f::Min(
                NSeq4f::Div(
                    TAccessorX::Seq(lhs),
                    NSeq4f::Max(TAccessorY::Seq(rhs), NSeq4f::TConstSeq4f(FloatEpsilon, rhs.Size()))),
                NSeq4f::TConstSeq4f(1.0f, rhs.Size()));
        }
    };

    using TFeaturesAddOp = TAddOp<TFeatureAccessor, TFeatureReadAccessor>;
    using TFeaturesWAddOp = TAddOp<TFeatureAccessor, TProductAccessor>;
    using TFeaturesW2AddOp = TAddOp<TFeatureAccessor, TSkewedProductAccessor<2>>;
    using TFeaturesMaxOp = TMaxOp<TFeatureAccessor, TFeatureReadAccessor>;
    using TFeaturesWMaxOp = TMaxOp<TFeatureAccessor, TProductAccessor>;
    using TFeaturesMinOp = TMinOp<TFeatureAccessor, TFeatureReadAccessor>;
    using TFeaturesWMinOp = TMinOp<TFeatureAccessor, TProductAccessor>;

    using TWeightsAddOp = TAddOp<TWeightAccessor, TWeightReadAccessor>;
    using TWeightsMaxOp = TMaxOp<TWeightAccessor, TWeightReadAccessor>;
    using TWeightsMinOp = TMinOp<TWeightAccessor, TWeightReadAccessor>;

    using TFeaturesNormW = TNormalizeOp<TFeatureReadAccessor, TWeightReadAccessor>;
    using TFeaturesNormF = TNormalizeOp<TFeatureReadAccessor, TFeatureReadAccessor>;

    using TWeightsNormW = TNormalizeOp<TWeightReadAccessor, TWeightReadAccessor>;
    using TWeightsNormF = TNormalizeOp<TWeightReadAccessor, TFeatureReadAccessor>;
} // NAggregator
} // NCore
} // NTextMachine
