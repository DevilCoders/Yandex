#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/flags.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NVectorMachine {
    namespace NAggregatorUtils {
        enum EThresholdStatType {
            TST_MAX_WEIGHT = 1,
            TST_AVG_WEIGHT = 2,
            TST_REL_COUNT = 4,
            TST_COUNT = 8,
            TST_SATURATED_WEIGHT_SUM = 16,
        };

        Y_DECLARE_FLAGS(EThresholdStatTypes, EThresholdStatType);
    }

    class TAvgTopScoreAggregator {
    private:
        TVector<float> Params;

    public:
        TAvgTopScoreAggregator(const TArrayRef<const float> params)
            : Params(params.begin(), params.end())
        {
        }

        void CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const;
    };

    class TAvgTopScoreWeightedAggregator {
    private:
        TVector<float> Params;

    public:
        TAvgTopScoreWeightedAggregator(const TArrayRef<const float> params)
            : Params(params.begin(), params.end())
        {
        }

        void CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const;
    };

    class TMinTopScoreAggregator {
    private:
        TVector<float> Params;

    public:
        TMinTopScoreAggregator(const TArrayRef<const float> params)
            : Params(params.begin(), params.end())
        {
        }

        void CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const;
    };

    class TMinTopScoreWeightedAggregator {
    private:
        TVector<float> Params;

    public:
        TMinTopScoreWeightedAggregator(const TArrayRef<const float> params)
            : Params(params.begin(), params.end())
        {
        }

        void CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const;
    };

    template <NAggregatorUtils::EThresholdStatTypes::TInt statTypes>
    class TScoreThresholdAggregator {
    public:

        TScoreThresholdAggregator(TArrayRef<const float> thresholds)
            : Thresholds(thresholds.begin(), thresholds.end())
        {
        }

        void CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const {
            Y_ENSURE(scores.size() == weights.size(), "Scores and weights lengths differ.");

            constexpr NAggregatorUtils::EThresholdStatTypes statsFlag = NAggregatorUtils::EThresholdStatTypes::FromBaseType(statTypes);
            if (scores.empty()) {
                Fill(result.begin(), result.begin() + Thresholds.size(), 0);
                return;
            }
            const size_t additive = 10;

            size_t pos = 0;
            for (float th : Thresholds) {
                bool reachedFirst = false;
                float max = 0.f;
                float sum = 0.f;
                size_t count = 0;
                for (size_t j = 0; j < scores.size(); j++) {
                    if (scores[j] >= th) {
                        if constexpr (statsFlag & NAggregatorUtils::TST_MAX_WEIGHT) {
                            max = reachedFirst ? ::Max(max, weights[j]) : weights[j];
                            reachedFirst = true;
                        }
                        if constexpr ((statsFlag & (NAggregatorUtils::TST_AVG_WEIGHT | NAggregatorUtils::TST_SATURATED_WEIGHT_SUM)) > 0) {
                            sum += weights[j];
                        }
                        if constexpr ((statsFlag &
                            (NAggregatorUtils::TST_AVG_WEIGHT | NAggregatorUtils::TST_REL_COUNT | NAggregatorUtils::TST_COUNT)) > 0)
                        {
                            ++count;
                        }
                    }
                }
                if constexpr (statsFlag & NAggregatorUtils::TST_MAX_WEIGHT) {
                    result[pos++] = max;
                }
                if constexpr (statsFlag & NAggregatorUtils::TST_AVG_WEIGHT) {
                    result[pos++] = count == 0 ? 0.f : sum / count;
                }
                if constexpr (statsFlag & NAggregatorUtils::TST_REL_COUNT) {
                    result[pos++] = 2.f * count / (count + scores.size());
                }
                if constexpr (statsFlag & NAggregatorUtils::TST_COUNT) {
                    result[pos++] = static_cast<float>(count) / (count + additive);
                }
                if constexpr (statsFlag & NAggregatorUtils::TST_SATURATED_WEIGHT_SUM) {
                    result[pos++] = sum / (sum + 1.f);
                }
            }
        }

    private:
        TVector<float> Thresholds;
    };

    class TAvgTopWeightAggregator {
    private:
        TVector<float> Params;

    public:
        TAvgTopWeightAggregator(const TArrayRef<const float> params)
            : Params(params.begin(), params.end())
        {
        }

        void CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const;
    };

    class TMinTopWeightAggregator {
    private:
        TVector<float> Params;

    public:
        TMinTopWeightAggregator(const TArrayRef<const float> params)
            : Params(params.begin(), params.end())
        {
        }

        void CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const;
    };

    class TIdentityAggregator {
    private:
        TVector<float> Params; // number of features

    public:
        TIdentityAggregator(const TArrayRef<const float> params)
            : Params(params.begin(), params.end())
        {
        }

        void CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const;
    };

    class TAvgTopScoreXWeightAggregator {
    public:
        TAvgTopScoreXWeightAggregator(const TArrayRef<const float> params)
            : Params(params.begin(), params.end())
        {
        }

        void CalcFeatures(const TArrayRef<const float>& scores, const TArrayRef<const float>& weights, TArrayRef<float> result) const;

    private:
        TVector<float> Params;
    };
} // namespace NVectorMachine
