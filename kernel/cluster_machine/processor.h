#pragma once

#include <kernel/cluster_machine/proto/clustering_params.pb.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NClustering {
    using TEmbed = TVector<float>;
    using TClustersWeights = TVector<float>;

    struct TResult {
        TVector<TEmbed> Centroids;          // assert(Centroids.shape == (n_centers, embed_size))   ; Initialized in IProcessor::Process
        TVector<TClustersWeights> Weights;  // assert(Weights.shape == (n_weight_types, n_centers)) ; Initialized in IProcessor::ProcessWeighted
        TVector<ui32> Labels;               // assert(Labels.shape == (n_embeddings))               ; Initialized in IProcessor::Process
        TVector<TVector<float>> Distances;  // assert(Distances.shape == (n_centers, n_points))     ; Initialized in IProcessor::Process
    };

    class IProcessor {
    public:
        virtual const NProto::TClusteringParams& GetClusteringParams() const = 0;

        /**
         * Required to fill Centroids in Process.
         * If you want to use base-class ProcessWeighted, also Required to fill Labels and Distances in Process.
         */
        virtual TResult Process(const TVector<TEmbed>& embeddings) const;

        virtual TResult ProcessWeighted(const TVector<TEmbed>& embeddings, const TVector<float>& weights) const;

        virtual ~IProcessor() = default;

    private:
        // legacy behaviour is using [ScaledSum,AvgClusterTop3] for 5 clusters and all types for 1 cluster
        void WeighClusters(TResult& clusters, const TVector<float>& weights) const;
    };

    THolder<IProcessor> CreateProcessor(const NProto::TClusteringParams& clusteringParams);
} // namespace NClustering
