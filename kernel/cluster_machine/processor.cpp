#include "make_clusters.h"
#include "processor.h"

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>

#include <cmath>


using TDotDistance = std::pair<ui32, float>;

template<>
struct TLess<TDotDistance> {
public:
    bool operator()(const TDotDistance& left, const TDotDistance& right) const {
        if (std::abs(left.second - right.second) < std::numeric_limits<float>::epsilon()) {
            return left.first < right.first;
        }
        return left.second < right.second;
    }
};

namespace {
    using namespace NClustering;

    TVector<TVector<ui32>> CalcNearestIndexes(const TVector<TVector<float>>& distances,
        const TVector<ui32>& labels)
    {
        TVector<TVector<TDotDistance>> preresult(distances.size());

        for (ui32 idx = 0; idx < labels.size(); ++idx) {
            preresult[labels[idx]].emplace_back(idx, distances[labels[idx]][idx]);
        }
        for (auto& clusterDots : preresult) {
            Sort(clusterDots.begin(), clusterDots.end(), TLess<TDotDistance>());
        }

        TVector<TVector<ui32>> result(Reserve(preresult.size()));
        for (const auto& centerPreresult : preresult) {
            TVector<ui32> centerResult(centerPreresult.size());
            for (ui32 i = 0; i < centerPreresult.size(); ++i) {
                centerResult[i] = centerPreresult[i].first;
            }
            result.push_back(centerResult);
        }
        return result;
    }

    TVector<TVector<float>> CalcCumSums(const TVector<TVector<ui32>>& nearest, const TVector<float>& weights) {
        TVector<TVector<float>> result(Reserve(nearest.size()));

        for (ui32 centerIdx = 0; centerIdx < nearest.size(); ++centerIdx) {
            TVector<float> cumSums(Reserve(nearest[centerIdx].size()));
            float sum = 0.f;
            for (const ui32 dotIdx : nearest[centerIdx]) {
                sum += weights[dotIdx];
                cumSums.push_back(sum);
            }
            result.push_back(cumSums);
        }
        return result;
    }

    TVector<TVector<float>> CalcAvgTops(const TVector<TVector<float>>& distances,
        const TVector<ui32>& labels, const TVector<float>& weights)
    {
        TVector<TVector<float>> result(distances.size());

        for (ui32 idx = 0; idx < labels.size(); ++idx) {
            result[labels[idx]].push_back(weights[idx]);
        }
        for (auto& avgTops : result) {
            Sort(avgTops.begin(), avgTops.end(), std::greater<float>());
            float sum = 0.f;
            for (ui32 i = 0; i < avgTops.size(); ++i) {
                sum += avgTops[i];
                avgTops[i] = sum / (i + 1);
            }
        }
        return result;
    }

    TVector<float> CalcDefaultWeights(const TResult& clusters, const TVector<float>& weights) {
        TVector<float> result(clusters.Centroids.size());
        for (ui32 i = 0; i < clusters.Labels.size(); ++i) {
            result[clusters.Labels[i]] = weights[i];
        }
        return result;
    }

    TVector<float> CalcScaledSumWeights(const TResult& clusters, const TVector<float>& weights) {
        if (weights.size() == clusters.Centroids.size()) {
            return CalcDefaultWeights(clusters, weights);
        }
        TVector<float> result(clusters.Centroids.size());
        float sum = 0.f;
        for (ui32 i = 0; i < clusters.Labels.size(); ++i) {
            result[clusters.Labels[i]] += weights[i];
            sum += weights[i] * (1.f + log(1.f + clusters.Distances[clusters.Labels[i]][i]));
        }
        for (auto& x : result) {
            x /= sum;
        }
        return result;
    }

    TVector<float> CalcLogCountWeights(const TResult& clusters) {
        TVector<float> result(clusters.Centroids.size());
        float sum = 0.f;
        for (ui32 i = 0; i < clusters.Labels.size(); ++i) {
            result[clusters.Labels[i]] += 1.f;
            sum += 1.f;
        }
        const float invLogSum = 1.f / log(1.f + sum);
        for (auto& x : result) {
            x = log(1.f + x) * invLogSum;
        }
        return result;
    }

    TVector<float> CalcAvgClusterTopWeights(const TResult& clusters, const TVector<float>& weights,
        const TVector<TVector<float>>& avgTopsAll, const ui32 k)
    {
        if (weights.size() == clusters.Centroids.size()) {
            return CalcDefaultWeights(clusters, weights);
        }
        TVector<float> result(Reserve(avgTopsAll.size()));
        for (const auto& avgTops : avgTopsAll) {
            const ui32 realK = ::Min<ui32>(k, avgTops.size());
            Y_ENSURE(realK > 0, "avgTopsAll[i] may not be empty");
            result.push_back(avgTops[realK - 1]);
        }
        return result;
    }

    TVector<float> CalcAvgNearestWeights(const TResult& clusters, const TVector<float>& weights,
        const TVector<TVector<float>>& cumSums, const ui32 k)
    {
        if (weights.size() == clusters.Centroids.size()) {
            return CalcDefaultWeights(clusters, weights);
        }
        TVector<float> result(Reserve(cumSums.size()));
        for (const auto& cumSum : cumSums) {
            const ui32 realK = ::Min<ui32>(k, cumSum.size());
            Y_ENSURE(realK > 0, "cumsums[i] may not be empty");
            result.push_back(cumSum[realK - 1] / realK);
        }
        return result;
    }
}


namespace NClustering {
    void IProcessor::WeighClusters(TResult& clusters, const TVector<float>& weights) const {
        Y_ENSURE(clusters.Centroids.size() <= weights.size());
        Y_ENSURE(clusters.Centroids.size() <= clusters.Labels.size(), "Labels have to be initialized");
        Y_ENSURE(clusters.Centroids.size() == clusters.Distances.size(), "Distances have to be initialized");

        clusters.Weights.clear();
        clusters.Weights.reserve(GetClusteringParams().GetWeightTypes().size());

        TVector<TVector<float>> cumSums;
        TVector<TVector<float>> avgTopsAll;

        for (const NProto::TClusterWeightsType& type : GetClusteringParams().GetWeightTypes()) {
            switch (type.GetWeightType()) {
                case NProto::EClusterWeightsType::Empty: // for no-weights aggregators
                    clusters.Weights.push_back(TVector<float>(clusters.Centroids.size()));
                    break;

                case NProto::EClusterWeightsType::ScaledSum:
                    clusters.Weights.push_back(CalcScaledSumWeights(clusters, weights));
                    break;

                case NProto::EClusterWeightsType::AvgNearestN:
                {
                    if (cumSums.empty()) {
                        const TVector<TVector<ui32>> nearest = CalcNearestIndexes(clusters.Distances, clusters.Labels);
                        cumSums = CalcCumSums(nearest, weights);
                    }

                    clusters.Weights.push_back(CalcAvgNearestWeights(clusters, weights, cumSums, type.GetNum()));
                    break;
                }
                case NProto::EClusterWeightsType::AvgClusterTopN:
                {
                    if (avgTopsAll.empty()) {
                        avgTopsAll = CalcAvgTops(clusters.Distances, clusters.Labels, weights);
                    }
                    clusters.Weights.push_back(CalcAvgClusterTopWeights(clusters, weights, avgTopsAll, type.GetNum()));
                    break;
                }
                case NProto::EClusterWeightsType::LogCount:
                {
                    clusters.Weights.push_back(CalcLogCountWeights(clusters));
                    break;
                }
            }
        }
    }

    TResult IProcessor::ProcessWeighted(const TVector<TEmbed>& embeddings, const TVector<float>& weights) const {
        Y_ENSURE(embeddings.size() == weights.size());

        TResult clusters = Process(embeddings);
        WeighClusters(clusters, weights);
        return clusters;
    }

    TResult IProcessor::Process(const TVector<TEmbed>& embeddings) const {
        return MakeClusters(embeddings, GetClusteringParams());
    }

    class TKMeansProcessor : public IProcessor {
    public:
        TKMeansProcessor(const NProto::TClusteringParams& clusteringParams)
            : ClusteringParams(clusteringParams)
        {
        }

        const NProto::TClusteringParams& GetClusteringParams() const override {
            return ClusteringParams;
        }

    private:
        const NProto::TClusteringParams ClusteringParams;
    };

    THolder<IProcessor> CreateProcessor(const NProto::TClusteringParams& clusteringParams) {
        switch (clusteringParams.GetTypeSpecificParamsCase()) {
            case NProto::TClusteringParams::kKMeansParams:
                return MakeHolder<TKMeansProcessor>(clusteringParams);

            default:
                ythrow yexception() << "Unknown clustering type" << Endl;
        }
    }
} // namespace NClustering
