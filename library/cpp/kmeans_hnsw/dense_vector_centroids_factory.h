#pragma once

#include <library/cpp/hnsw/index_builder/dense_vector_storage.h>

#include <util/generic/buffer.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <cmath>

namespace NKmeansHnsw {
    template <class TVectorComponent>
    class TDenseVectorCentroidsFactory {
        using TDenseVector = TVectorComponent* const;

    public:
        TDenseVectorCentroidsFactory(const size_t dimension)
            : Dimension(dimension)
        {
        }

        void Reset(const size_t numClusters) {
            NumClusters = numClusters;

            ClusterCentroidsBuffer.Resize(NumClusters * Dimension * sizeof(TVectorComponent));
            CentroidsData = reinterpret_cast<TVectorComponent*>(ClusterCentroidsBuffer.Data());
            new (CentroidsData) TVectorComponent[NumClusters * Dimension]();

            ClusterItemCounts.assign(NumClusters, 0);
        }

        void AddItemToCluster(const TVectorComponent* item, const size_t clusterId) {
            Y_VERIFY(Inited());
            Y_ASSERT(item);

            ++ClusterItemCounts[clusterId];

            TDenseVector cluster = GetClusterVector(clusterId);
            for (size_t i = 0; i < Dimension; ++i) {
                cluster[i] += item[i];
            }
        }

        NHnsw::TDenseVectorStorage<TVectorComponent> GetClusterCentroids() {
            Y_VERIFY(Inited());

            for (size_t clusterId = 0; clusterId < ClusterItemCounts.size(); ++clusterId) {
                if (!ClusterItemCounts[clusterId]) {
                    continue;
                }

                TDenseVector cluster = GetClusterVector(clusterId);

                for (size_t i = 0; i < Dimension; ++i) {
                    cluster[i] /= ClusterItemCounts[clusterId];
                }
            }

            return NHnsw::TDenseVectorStorage<TVectorComponent>(TBlob::FromBuffer(ClusterCentroidsBuffer), Dimension);
        }

    private:
        TVectorComponent* GetClusterVector(const size_t clusterId) const {
            Y_ASSERT(Inited());
            Y_ASSERT(clusterId < NumClusters);
            return CentroidsData + clusterId * Dimension;
        }

        bool Inited() const {
            return CentroidsData != nullptr;
        }

    private:
        const size_t Dimension = 0;
        size_t NumClusters = 0;
        TVector<ui32> ClusterItemCounts;
        TBuffer ClusterCentroidsBuffer;
        TVectorComponent* CentroidsData = nullptr;
    };

}
