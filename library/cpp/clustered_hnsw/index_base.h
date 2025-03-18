#pragma once

#include <library/cpp/containers/top_keeper/top_keeper.h>
#include <library/cpp/hnsw/index/index_base.h>

#include <util/generic/vector.h>
#include <util/generic/hash_set.h>
#include <util/generic/algorithm.h>
#include <util/system/unaligned_mem.h>

namespace NClusteredHnsw {
    template <class TDistance>
    struct TStatistics {
        // Intended for use with a call-counting distance to profile the number of calls on HNSW search phase
        TDistance DistanceAfterHnswSearch;

        // Total number of vectors in candidate clusters, including duplicates
        ui32 NumClusterCandidates = 0;

        // Number of distance calls for cluster candidates; can be less than NumClusterCandidates when clusters are overlapping
        ui32 NumClusterCandidatesConsidered = 0;
    };

    template <class TClustersItemStorage, class TItemStorageDeserializer, class TItem = typename TClustersItemStorage::TItem>
    class TClusteredHnswIndexBase {
    public:
        template <class TDistanceResult>
        using TNeighbor = NHnsw::THnswIndexBase::TNeighbor<TDistanceResult>;

        explicit TClusteredHnswIndexBase(const TBlob& blob)
            : ClustersHnswIndex(GetHnswSubblob(blob))
        {
            Reset(blob);
        }

        explicit TClusteredHnswIndexBase(const TString& filename)
            : TClusteredHnswIndexBase(TBlob::PrechargedFromFile(filename))
        {
        }

        template <class TItemStorage,
                  class TDistance,
                  class TDistanceResult = typename TDistance::TResult,
                  class TDistanceLess = typename TDistance::TLess,
                  bool sortResults = false>
        TVector<TNeighbor<TDistanceResult>> GetNearestNeighbors(
            const TItem& query,
            const size_t topSize,
            const size_t clusterTopSize,
            const size_t clusterNeighborhoodSize,
            const TItemStorage& itemStorage,
            const TDistance& distance = {},
            const TDistanceLess& distanceLess = {},
            TStatistics<TDistance>* const statistics = nullptr) const {
            const TVector<TNeighbor<TDistanceResult>> neighborClusters =
                ClustersHnswIndex.GetNearestNeighbors<TClustersItemStorage, TDistance, TDistanceResult, TDistanceLess, TItem>(
                    query,
                    clusterTopSize,
                    Max<size_t>(clusterTopSize, clusterNeighborhoodSize),
                    *ClustersStorage.Get(),
                    distance, distanceLess);

            if (statistics) {
                // TDistance can be immutable (e.g. if it has const members), so use a hack here
                statistics->DistanceAfterHnswSearch.~TDistance();
                new (&statistics->DistanceAfterHnswSearch) TDistance(distance);
            }

            ui32 totalClustersSize = 0;
            for (size_t i = 0; i < neighborClusters.size(); ++i) {
                const ui32 clusterId = neighborClusters[i].Id;
                totalClustersSize += GetClusterSize(clusterId);
            }

            TDenseHashSet<ui32> usedIds(Max<ui32>(), totalClustersSize * 2);

            const auto neighborGreater = [&](const TNeighbor<TDistanceResult>& lhs, const TNeighbor<TDistanceResult>& rhs) {
                return distanceLess(lhs.Dist, rhs.Dist);
            };

            TTopKeeper<TNeighbor<TDistanceResult>, decltype(neighborGreater), sortResults> currentTop(topSize, neighborGreater);

            ui32 numCandidatesConsidered = 0;

            for (size_t neighborId = 0; neighborId < neighborClusters.size(); ++neighborId) {
                const ui32 clusterId = neighborClusters[neighborId].Id;
                const ui32* const ids = GetClusterIds(clusterId);

                for (size_t i = 0; i < GetClusterSize(clusterId); ++i) {
                    if (usedIds.Insert(ids[i])) {
                        TNeighbor<TDistanceResult> neighbor{distance(query, itemStorage.GetItem(ids[i])), (ui32)ids[i]};
                        currentTop.Insert(neighbor);
                        ++numCandidatesConsidered;
                    }
                }
            }

            if (statistics) {
                statistics->NumClusterCandidatesConsidered = numCandidatesConsidered;
                statistics->NumClusterCandidates = totalClustersSize;
            }

            currentTop.Finalize();

            return currentTop.GetInternal();
        }

        void Reset(const TBlob& blob) {
            Data = blob;

            const ui8* const data = reinterpret_cast<const ui8*>(Data.Begin());
            size_t offset = 0;

            const size_t clustersHnswIndexSize = ReadUnaligned<ui64>(data + offset);
            offset += sizeof(ui64) + clustersHnswIndexSize;

            const size_t clustersStorageSize = ReadUnaligned<ui64>(data + offset);
            offset += sizeof(ui64);

            ClustersStorage = MakeHolder<TClustersItemStorage>(TItemStorageDeserializer::Load(Data.SubBlob(offset, offset + clustersStorageSize)));
            offset += clustersStorageSize;

            NumClusters = ReadUnaligned<ui32>(data + offset);
            offset += sizeof(ui32);

            ClusterOffsets = reinterpret_cast<const ui32*>(data + offset);
            offset += (NumClusters + 1) * sizeof(ui32);

            ClusterIds = reinterpret_cast<const ui32*>(data + offset);
            offset += ClusterOffsets[NumClusters] * sizeof(ui32);

            Y_VERIFY(offset == blob.Size());
        }

        ui32 GetNumClusters() const {
            return NumClusters;
        }

        ui32 GetClusterSize(ui32 clusterId) const {
            Y_ASSERT(clusterId < NumClusters);
            return ClusterOffsets[clusterId + 1] - ClusterOffsets[clusterId];
        }

        const ui32* GetClusterIds(ui32 clusterId) const {
            Y_ASSERT(clusterId < NumClusters);
            return ClusterIds + ClusterOffsets[clusterId];
        }

    private:
        static TBlob GetHnswSubblob(const TBlob& blob) {
            const size_t clustersHnswIndexSize = *reinterpret_cast<const ui64*>(blob.Begin());
            return blob.SubBlob(sizeof(ui64), sizeof(ui64) + clustersHnswIndexSize);
        }

    private:
        TBlob Data;
        NHnsw::THnswIndexBase ClustersHnswIndex;
        THolder<TClustersItemStorage> ClustersStorage;
        ui32 NumClusters;
        const ui32* ClusterOffsets;
        const ui32* ClusterIds;
    };

}
