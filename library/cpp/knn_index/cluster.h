#pragma once

#include "constants.h"
#include "item.h"

namespace NKNNIndex {
    /**
     * @brief Class for representing of inner structure of KNNIndex.
     * Provides methods for getting items it contains and
     * coordinates of the cluster center.
     */
    template <class Searcher>
    class TCluster {
    public:
        using TSearcher = Searcher;
        using TFeatureType = typename TSearcher::TFeatureType;

        TCluster(const TSearcher* searcher, ui32 clusterId)
            : Searcher_(searcher)
            , ClusterId_(clusterId)
        {
            Y_ASSERT(Searcher_);
            Y_ASSERT(clusterId < Searcher_->ClusterCount());
            ClusterOffset_ = Searcher_->ClusterOffset_[clusterId];
            ItemCount_ = Searcher_->ClusterOffset_[clusterId + 1] - ClusterOffset_;
            Centroid_ = Searcher_->Cluster_ + clusterId * Searcher_->AlignedDimension_;
        }

        const TFeatureType* Centroid() const {
            Y_ASSERT(Centroid_);
            return Centroid_;
        }

        TItem<TSearcher> Item(size_t index) const {
            Y_ASSERT(index < ItemCount());
            return Searcher_->Item(ClusterOffset_ + index);
        }

        size_t ItemCount() const {
            return ItemCount_;
        }

        size_t Dimension() const {
            return Searcher_->Dimension();
        }

    private:
        const TSearcher* Searcher_ = nullptr;
        const TFeatureType* Centroid_ = nullptr;
        ui32 ClusterId_ = 0;
        ui32 ClusterOffset_ = 0;
        ui32 ItemCount_ = 0;
    };

}
