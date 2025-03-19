#pragma once
#include <tuple>
#include <util/generic/array_ref.h>
#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>
#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <util/generic/queue.h>

namespace NWeighedSequenceFeatures {

namespace NImpl {


template<size_t cur, class THead, class ... TTail>
constexpr bool CheckOrdering() {
    static_assert(THead::Level <= cur);
    if constexpr (sizeof...(TTail) > 0) {
        return CheckOrdering<THead::Level, TTail...>();
    }
    return true;
}


template<class TFilter, size_t curId = 0, class TTuple>
constexpr const TFilter& GetStateFieldImpl(const TTuple& t) {
    static_assert(curId < std::tuple_size_v<TTuple>);
    const auto& current = std::get<curId>(t);
    if constexpr (std::is_same_v<const TFilter&, decltype(current)>) {
        return current;
    } else {
        return GetStateFieldImpl<TFilter, curId + 1, TTuple>(t);
    }
}

template<class TFilter, class TW>
constexpr const TFilter& GetStateField(const TW& t) {
    return GetStateFieldImpl<TFilter>(t.GetState());
}

template<class TAggregator, class TFunctor>
struct TSimpleAggregatorUnit {
    static constexpr size_t Level = 0;

    float Value = 0;
    float GetMainField() const {
        return Value;
    }

    template<class T>
    void PrepareState(
        const T&,
        TArrayRef<const float> weights,
        TArrayRef<const float> features) {
        Value = 0; // reset

        float res = TFunctor::GetVal(weights.front(), features.front());
        for (size_t i : xrange<size_t>(1, weights.size())) {
            res = TAggregator::Merge(res, TFunctor::GetVal(weights[i], features[i]));
        }
        Value = res;
    }
};


struct TSumMerger {
    static float Merge(float a, float b) {
        return a + b;
    }
};

struct TMaxMerger {
    static float Merge(float a, float b) {
        return Max(a, b);
    }
};

struct TMinMerger {
    static float Merge(float a, float b) {
        return Min(a, b);
    }
};

struct TWeightFunctor {
    static float GetVal(float w, [[maybe_unused]] float f) {
        return w;
    }
};

struct TFeatureFunctor {
    static float GetVal([[maybe_unused]] float w, float f) {
        return f;
    }
};

struct TWeightedFeatureFunctor {
    static float GetVal(float w, float f) {
        return w * f;
    }
};

template<size_t TopSize>
struct TTopByFeaturesHolder {
    struct TNode {
        float Weight = 0;
        float Feature = 0;

        bool operator<(const TNode& b) const {
            return Feature > b.Feature; //Holding top
        }
    };

    using TContainer = TStackOnlyVec<TNode, TopSize + 1>;
    using TQueue = TPriorityQueue<TNode, TContainer>;

    TStackOnlyVec<float, TopSize> Weights;
    TStackOnlyVec<float, TopSize> Features;

    void Collect(
        TArrayRef<const float> weights,
        TArrayRef<const float> features)
    {
        TQueue top;
        for (auto i : xrange(weights.size())) {
            top.push({weights[i], features[i]});
            if (top.size() > TopSize) {
                top.pop();
            }
        }

        for (TNode c : top.Container()) {
            Weights.push_back(c.Weight);
            Features.push_back(c.Feature);
        }
    }
};

template<class TNominator, class TDenominator, class TWidget>
inline float CalcNormedValue(const TWidget& widget) {
    float nominator = NImpl::GetStateField<TNominator>(widget).GetMainField();
    float denominator = NImpl::GetStateField<TDenominator>(widget).GetMainField();

    if (denominator > 0) {
        Y_ASSERT(nominator <= denominator);
        return nominator / denominator;
    } else {
        return 0;
    }
}

}

template<class ... TUnits>
class TWidget {
    using TState = std::tuple<TUnits...>;
    TState State = {};
    static_assert(NImpl::CheckOrdering<0, TUnits...>());
    static_assert(std::is_trivially_destructible_v<TState>);
public:

    const TState& GetState() const {
        return State;
    }

    void PrepareState(
        TArrayRef<const float> weights,
        TArrayRef<const float> features)
    {
        Y_ENSURE(weights.size() == features.size());
        for (auto i : xrange(weights.size())) {
            Y_ENSURE(weights[i] >= 0.f && weights[i] <= 1.f, "weights not in [0,1]" << weights[i] << " " << i);
            Y_ENSURE(features[i] >= 0.f && features[i] <= 1.f, "features not in [0,1]" << features[i] << " " << i);
        }

        ForEach(State, [this, weights, features](auto& unit) {
            unit.PrepareState(std::as_const(this->State), weights, features);
        });
    }
};

struct TUnitMaxWeihgt : public NImpl::TSimpleAggregatorUnit<NImpl::TMaxMerger, NImpl::TWeightFunctor> {};
struct TUnitMaxFeature : public NImpl::TSimpleAggregatorUnit<NImpl::TMaxMerger, NImpl::TFeatureFunctor> {};
struct TUnitMaxWeightedFeature : public NImpl::TSimpleAggregatorUnit<NImpl::TMaxMerger, NImpl::TWeightedFeatureFunctor> {};
struct TUnitMinWeightedFeature : public NImpl::TSimpleAggregatorUnit<NImpl::TMinMerger, NImpl::TWeightedFeatureFunctor> {};
struct TUnitFeaturesValuesSum : public NImpl::TSimpleAggregatorUnit<NImpl::TSumMerger, NImpl::TFeatureFunctor> {};
struct TUnitWeightedFeaturesSum : public NImpl::TSimpleAggregatorUnit<NImpl::TSumMerger, NImpl::TWeightedFeatureFunctor> {};
struct TUnitWeightsValuesSum : public NImpl::TSimpleAggregatorUnit<NImpl::TSumMerger, NImpl::TWeightFunctor> {};

struct TUnitCount {
    static constexpr size_t Level = 0;
    size_t Count = 0;
    float GetMainField() const {
        return Count;
    }

    template<class T>
    void PrepareState(
        const T&,
        TArrayRef<const float> weights,
        TArrayRef<const float>)
    {
        Count = weights.size();
    }
};


template<class TW>
inline float CalcMaxWF(const TW& widget) {
    return NImpl::GetStateField<TUnitMaxWeightedFeature>(widget).GetMainField();
}

template<class TW>
inline float CalcMaxWFNormedMaxW(const TW& widget) {
    return NImpl::CalcNormedValue<TUnitMaxWeightedFeature, TUnitMaxWeihgt>(widget);
}

template<class TW>
inline float CalcSumFNormedCount(const TW& widget) {
    return NImpl::CalcNormedValue<TUnitFeaturesValuesSum, TUnitCount>(widget);
}

template<class TW>
inline float CalcMinWFNormedMaxW(const TW& widget) {
    return NImpl::CalcNormedValue<TUnitMinWeightedFeature, TUnitMaxWeihgt>(widget);
}

template<class TW>
inline float CalcMinWF(const TW& widget) {
    return NImpl::GetStateField<TUnitMinWeightedFeature>(widget).GetMainField();
}

template<class TW>
inline float CalcSumWFNormedSumW(const TW& widget) {
    return NImpl::CalcNormedValue<TUnitWeightedFeaturesSum, TUnitWeightsValuesSum>(widget);
}

template<size_t TopSize, class TW>
inline void ApplyThroughTopFeaturesFilter(
    TW& w,
    TArrayRef<const float> weights,
    TArrayRef<const float> features)
{
    NImpl::TTopByFeaturesHolder<TopSize> topHolder;
    topHolder.Collect(weights, features);
    w.PrepareState(topHolder.Weights, topHolder.Features);
}

}
