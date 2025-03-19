#include "aggregators.h"

#include <library/cpp/containers/top_keeper/top_keeper.h>

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>
#include <util/generic/ymath.h>

namespace {
    inline void CalcAvgTops(TVector<float>& tops, const size_t maxTopSize) noexcept {
        float sum = 0.f;
        for (size_t i = 0; i < maxTopSize; ++i) {
            sum += tops[i];
            tops[i] = sum / (i + 1);
        }
    }

    inline size_t GetIndexByParam(const float top, const size_t len) noexcept {
        if (top >= 0.f && top <= 1.f) {
            size_t index = static_cast<size_t>(len * top);
            if (index > 0) {
                --index;
            }
            return index;
        } else { // if top > 1.f, i.e. top size is explicitly specified as an Integer
            return ::Min(static_cast<size_t>(top), len) - 1;
        }
    }

    inline TVector<size_t> ParamsToSizeArray(const TArrayRef<const float>& params, const size_t len) noexcept {
        TVector<size_t> result(params.size());
        for (size_t i = 0; i < params.size(); ++i) {
            result[i] = GetIndexByParam(params[i], len);
        }
        return result;
    }

    inline void GetElementsForIdxs(const TArrayRef<const float>& elements, const TVector<size_t>& indexes, TArrayRef<float> result) noexcept {
        for (size_t i = 0; i < indexes.size(); ++i) {
            result[i] = elements[indexes[i]];
        }
    }

    inline size_t GetMaxTopSizeByIndexes(const TVector<size_t>& indexes) noexcept {
        return indexes.empty() ? 0 : *MaxElement(indexes.begin(), indexes.end()) + 1;
    }

    struct TCompareByDotProduct {
        TCompareByDotProduct(const TArrayRef<const float>& dotProducts)
            : DotProducts(dotProducts)
        {
        }

        bool operator()(const size_t lhs, const size_t rhs) const noexcept {
            return DotProducts[lhs] > DotProducts[rhs];
        }

    private:
        const TArrayRef<const float>& DotProducts;
    };
}

namespace NVectorMachine {
    void TAvgTopScoreAggregator::CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const {
        Y_UNUSED(weights);
        Y_ENSURE(result.size() == Params.size());
        if (scores.empty()) {
            Fill(result.begin(), result.end(), 0);
            return;
        }
        TVector<size_t> topSizes = ParamsToSizeArray(Params, scores.size());
        const size_t maxTopSize = GetMaxTopSizeByIndexes(topSizes);

        TVector<float> tops(scores.begin(), scores.end());
        PartialSort(tops.begin(), tops.begin() + maxTopSize, tops.end(), std::greater<float>());

        CalcAvgTops(tops, maxTopSize);
        GetElementsForIdxs(tops, topSizes, result);

    }

    void TAvgTopScoreWeightedAggregator::CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const {
        Y_ENSURE(scores.size() == weights.size(), "Scores and weights lengths differ.");
        Y_ENSURE(result.size() == Params.size());
        if (scores.empty()) {
            Fill(result.begin(), result.end(), 0);
            return;
        }
        TVector<size_t> topSizes = ParamsToSizeArray(Params, scores.size());
        const size_t maxTopSize = GetMaxTopSizeByIndexes(topSizes);

        TVector<float> tops(scores.size());
        for (size_t i = 0; i < scores.size(); ++i) {
            tops[i] = scores[i] * weights[i];
        }
        PartialSort(tops.begin(), tops.begin() + maxTopSize, tops.end(), std::greater<float>());

        CalcAvgTops(tops, maxTopSize);
        GetElementsForIdxs(tops, topSizes, result);
    }

    void TMinTopScoreAggregator::CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const {
        Y_UNUSED(weights);
        Y_ENSURE(result.size() == Params.size());
        if (scores.empty()) {
            Fill(result.begin(), result.end(), 0);
            return;
        }
        TVector<size_t> topSizes = ParamsToSizeArray(Params, scores.size());
        const size_t maxTopSize = GetMaxTopSizeByIndexes(topSizes);

        TVector<float> tops(scores.begin(), scores.end());
        PartialSort(tops.begin(), tops.begin() + maxTopSize, tops.end(), std::greater<float>());

        GetElementsForIdxs(tops, topSizes, result);
    }

    void TMinTopScoreWeightedAggregator::CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const {
        Y_ENSURE(scores.size() == weights.size(), "Scores and weights lengths differ.");
        Y_ENSURE(result.size() == Params.size());
        if (scores.empty()) {
            Fill(result.begin(), result.end(), 0);
            return;
        }
        TVector<size_t> topSizes = ParamsToSizeArray(Params, scores.size());
        const size_t maxTopSize = GetMaxTopSizeByIndexes(topSizes);

        TVector<float> tops(scores.size());
        for (size_t i = 0; i < scores.size(); ++i) {
            tops[i] = scores[i] * weights[i];
        }
        PartialSort(tops.begin(), tops.begin() + maxTopSize, tops.end(), std::greater<float>());

        GetElementsForIdxs(tops, topSizes, result);
    }

    void TAvgTopWeightAggregator::CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const {
        Y_UNUSED(scores);
        Y_ENSURE(result.size() == Params.size());
        if (weights.empty()) {
            Fill(result.begin(), result.end(), 0);
            return;
        }
        TVector<size_t> topSizes = ParamsToSizeArray(Params, scores.size());
        const size_t maxTopSize = GetMaxTopSizeByIndexes(topSizes);

        TVector<float> tops(weights.begin(), weights.end());
        PartialSort(tops.begin(), tops.begin() + maxTopSize, tops.end(), std::greater<float>());

        CalcAvgTops(tops, maxTopSize);

        GetElementsForIdxs(tops, topSizes, result);
    }

    void TMinTopWeightAggregator::CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const {
        Y_UNUSED(scores);
        Y_ENSURE(result.size() == Params.size());
        if (weights.empty()) {
            Fill(result.begin(), result.end(), 0);
            return;
        }
        TVector<size_t> topSizes = ParamsToSizeArray(Params, scores.size());
        const size_t maxTopSize = GetMaxTopSizeByIndexes(topSizes);

        TVector<float> tops(weights.begin(), weights.end());
        PartialSort(tops.begin(), tops.begin() + maxTopSize, tops.end(), std::greater<float>());

        GetElementsForIdxs(tops, topSizes, result);
    }

    void TIdentityAggregator::CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const {
        Y_UNUSED(weights);
        Y_ENSURE(result.size() == Params.size());
        Y_ENSURE(scores.size() == Params.size());
        for (ui64 i = 0; i < scores.size(); ++i) {
            result[i] = scores[i];
        }
    }

    void TAvgTopScoreXWeightAggregator::CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const {
        Y_ENSURE(result.size() == Params.size());
        if (scores.empty()) {
            return;
        }
        TVector<size_t> topSizes = ParamsToSizeArray(Params, scores.size());
        const size_t maxTopSize = GetMaxTopSizeByIndexes(topSizes);

        TTopKeeper<size_t, TCompareByDotProduct> topKeeper(maxTopSize, TCompareByDotProduct(scores));
        for (size_t i = 0; i < scores.size(); ++i) {
            topKeeper.Insert(i);
        }
        TVector<float> tops(Reserve(maxTopSize));
        float prodSum = 0.f;
        float weightSum = 0.f;
        for (const size_t i : topKeeper.GetInternal()) {
            prodSum += scores[i] * weights[i];
            weightSum += weights[i];
            float topValue = 0.f;
            if (!FuzzyEquals(1.f + weightSum, 1.f)) {
                topValue = prodSum / weightSum;
            }
            tops.emplace_back(topValue);
        }

        GetElementsForIdxs(tops, topSizes, result);
    }
} // namespace NVectorMachine
