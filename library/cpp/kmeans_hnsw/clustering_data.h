#pragma once

#include <util/generic/vector.h>

namespace NKmeansHnsw {
    template <class TItemStorage>
    struct TClusteringData {
        // Storage for cluster centroids
        TItemStorage Clusters;

        // Cluster index -> Indices of vectors in the cluster
        TVector<TVector<ui32>> ClusterIds;
    };

}
