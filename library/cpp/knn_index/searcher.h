#pragma once

#include "cluster.h"
#include "constants.h"
#include "distance.h"
#include "document.h"
#include "search_item_result.h"

#include <library/cpp/knn_index/index_info.pb.h>

#include <library/cpp/containers/limited_heap/limited_heap.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/system/align.h>
#include <util/system/types.h>

namespace NKNNIndex {
    /**
     * @brief Class that provides access to KNNIndex of document items.
     * It consists of clusters, cluster consists of items(each item has DocId, ItemId)
     * Each item is a fixed length vector of FeatureType.
     *
     * Provides method for knn-search and retrieving clusters and documents features.
     * Detailed data-description of underlying blob is in TWriter-class.
     * Also any user-defined proto-structure can be stored in this index for saving version or smth like that.
     *
     * @code
     *  TSearcher<i8> searcher(path);
     *  for (const auto& item : searcher.FindNearestItems<EDistanceType::DotProduct>(
     *      query, topClusterCount, topItemCount)
     *  {
     *      Cerr << item.DocumentId() << ' ' << item.ItemId() << ' ' << item.Distance() << Endl;
     *  }
     *  auto cluster = searcher.Cluster(clusterId);
     *  for (size_t i = 0; i < cluster.ItemCount(); ++i) {
     *      auto item = cluster.Item(i);
     *      Cerr << item.DocumentId() << ' ' << item.ItemId() << ' ' << DotProduct(item.Item(), query, length) << Endl;
     *  }
     *  auto document = searcher.Document(documentId);
     *  if (document.ItemCount()) {
     *      Cerr << L2Distance(document.Item(0), query, length) << Endl;
     *  }
     * @endcode
     *
     * @tparam FeatureType                  Underylying type of item fixed-length vectors(ui8, i8, float, ...)
     */
    template <class FeatureType, class UserIndexInfoPB = TDefaultUserIndexInfo>
    class TSearcher {
    public:
        using TFeatureType = FeatureType;
        using TUserIndexInfo = UserIndexInfoPB;

        explicit TSearcher(const TString& path) {
            Reset(TBlob::PrechargedFromFile(path));
        }

        explicit TSearcher(const TBlob& blob) {
            Reset(blob);
        }

        const TUserIndexInfo& UserIndexInfo() const {
            return UserIndexInfo_;
        }

        size_t Dimension() const {
            Y_ASSERT(Info_.HasDimension());
            return Info_.GetDimension();
        }

        template <EDistanceType distanceType>
        TVector<TSearchItemResult<TFeatureType, distanceType>> FindNearestItems(
            const TFeatureType* query, size_t topClusterCount, size_t topItemCount) const {
            using TItemResult = TSearchItemResult<TFeatureType, distanceType>;
            using TDistance = typename TItemResult::TDistance;

            TDistanceCalculator<TFeatureType, distanceType> distanceCalculator;
            TDistanceLess<TFeatureType, distanceType> distanceLess;
            auto itemLess = [&](const TItemResult& l, const TItemResult& r) {
                return distanceLess(l.Distance(), r.Distance());
            };
            TLimitedHeap<TItemResult, decltype(itemLess)> top(topItemCount, itemLess);
            for (const ui32 clusterId : FindNearestClusters<distanceType>(query, topClusterCount)) {
                for (ui32 itemPos = ClusterOffset_[clusterId]; itemPos < ClusterOffset_[clusterId + 1]; ++itemPos) {
                    TDistance distance = distanceCalculator(query, Item_ + AlignedDimension_ * itemPos, Dimension());
                    if (top.GetSize() < topItemCount || distanceLess(distance, top.GetMin().Distance())) {
                        top.Insert(TItemResult(DocumentId_[itemPos], ItemId_[itemPos], distance));
                    }
                }
            }
            TVector<TItemResult> result;
            for (; !top.IsEmpty(); top.PopMin()) {
                result.push_back(top.GetMin());
            }
            std::reverse(result.begin(), result.end());
            return result;
        }

        TDocument<TSearcher> Document(ui32 documentId) const {
            if (documentId > Info_.GetMaxDocumentId()) {
                return TDocument<TSearcher>(this, documentId, /*exists=*/false);
            } else {
                return TDocument<TSearcher>(this, documentId, /*exists=*/true);
            }
        }

        TItem<TSearcher> Item(size_t index) const {
            Y_ASSERT(index < ItemCount());
            return TItem<TSearcher>(this, index);
        }

        size_t ItemCount() const {
            Y_ASSERT(Info_.HasItemCount());
            return Info_.GetItemCount();
        }

        TCluster<TSearcher> Cluster(size_t index) const {
            Y_ASSERT(index < ClusterCount());
            return TCluster<TSearcher>(this, index);
        }

        size_t ClusterCount() const {
            Y_ASSERT(Info_.HasClusterCount());
            return Info_.GetClusterCount();
        }

    private:
        template <EDistanceType distanceType>
        struct TClusterDistance {
            using TDistance = typename TDistanceCalculator<TFeatureType, distanceType>::TResult;
            ui32 ClusterId = 0;
            TDistance Distance = 0;
        };

        template <EDistanceType distanceType>
        TVector<ui32> FindNearestClusters(const TFeatureType* query, size_t topClusterCount) const {
            TDistanceCalculator<TFeatureType, distanceType> distanceCalculator;
            TDistanceLess<TFeatureType, distanceType> distanceLess;
            auto clusterComparator = [&](const auto& l, const auto& r) {
                return distanceLess(l.Distance, r.Distance);
            };
            TLimitedHeap<TClusterDistance<distanceType>, decltype(clusterComparator)> top(
                topClusterCount, clusterComparator);
            for (ui32 clusterId = 0; clusterId < ClusterCount(); ++clusterId) {
                TClusterDistance<distanceType> cluster;
                cluster.ClusterId = clusterId;
                cluster.Distance = distanceCalculator(query, Cluster_ + clusterId * AlignedDimension_, Dimension());
                top.Insert(cluster);
            }
            TVector<ui32> result;
            for (; !top.IsEmpty(); top.PopMin()) {
                result.push_back(top.GetMin().ClusterId);
            }
            return result;
        }

        void Reset(const TBlob& blob) {
            Blob_ = blob;
            Y_VERIFY(Blob_.Size() >= sizeof(ui32));
            const ui32& headerSize = *(const ui32*)(Blob_.AsCharPtr() + Blob_.Size() - sizeof(ui32));
            const char* ptr = Blob_.AsCharPtr() + Blob_.Size() - headerSize - sizeof(ui32);

            /* Read TIndexInfo-proto structure */
            const ui32& infoSize = *(const ui32*)ptr;
            Y_PROTOBUF_SUPPRESS_NODISCARD Info_.ParseFromArray(ptr + sizeof(infoSize), infoSize);
            ptr += sizeof(infoSize) + infoSize;

            /* Read TUserIndexInfo-proto structure */
            const ui32& userInfoSize = *(const ui32*)ptr;
            Y_PROTOBUF_SUPPRESS_NODISCARD UserIndexInfo_.ParseFromArray(ptr + sizeof(userInfoSize), userInfoSize);
            ptr += sizeof(userInfoSize) + userInfoSize;

            /* Fill offsets and ids array */
            ClusterOffset_ = (const ui32*)ptr;
            ptr += (Info_.GetClusterCount() + 1) * sizeof(ClusterOffset_[0]);
            ItemId_ = (const ui32*)ptr;
            ptr += Info_.GetItemCount() * sizeof(ItemId_[0]);
            DocumentId_ = (const ui32*)ptr;
            ptr += Info_.GetItemCount() * sizeof(DocumentId_[0]);
            DocumentItem_ = (const ui32*)ptr;
            ptr += (Info_.GetItemCount()) * sizeof(DocumentItem_[0]);
            DocumentOffset_ = (const ui32*)ptr;
            ptr += (Info_.GetMaxDocumentId() + 2) * sizeof(DocumentOffset_[0]);
            Y_VERIFY(ptr + sizeof(ui32) == Blob_.AsCharPtr() + Blob_.Size());

            /* Fill feature vectors data */
            AlignedDimension_ = AlignUp<size_t>(Info_.GetDimension() * sizeof(TFeatureType), DataAlignment) / sizeof(TFeatureType);
            Item_ = (const TFeatureType*)Blob_.AsCharPtr();
            Cluster_ = (const TFeatureType*)(Blob_.AsCharPtr() + Info_.GetItemCount() * AlignedDimension_ * sizeof(TFeatureType));
        }

    private:
        TBlob Blob_;
        TIndexInfo Info_;
        TUserIndexInfo UserIndexInfo_;
        size_t AlignedDimension_ = 0;
        const ui32* ClusterOffset_ = nullptr;  /* for getting of all items of cluster */
        const ui32* ItemId_ = nullptr;         /* indexed by ItemPosition */
        const ui32* DocumentId_ = nullptr;     /* indexed by ItemPosition */
        const ui32* DocumentItem_ = nullptr;   /* flat array of document item-positions sorted by docId */
        const ui32* DocumentOffset_ = nullptr; /* DocumentItem_[DocumentOffset_[docId]] - first item-pos of doc */
        const TFeatureType* Cluster_ = nullptr;
        const TFeatureType* Item_ = nullptr;

        friend class TItem<TSearcher>;
        friend class TDocument<TSearcher>;
        friend class TCluster<TSearcher>;
    };

}
