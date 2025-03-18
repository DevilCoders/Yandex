#pragma once

#include <util/system/types.h>
#include <util/system/yassert.h>

namespace NKNNIndex {
    /**
     * Class for item-in-index representation.
     * Provides DocumentId, ItemId and Item(feature vector)
     */
    template <class Searcher>
    class TItem {
    public:
        using TSearcher = Searcher;
        using TFeatureType = typename TSearcher::TFeatureType;

        TItem(const TSearcher* searcher, ui32 itemPos)
            : Searcher_(searcher)
            , ItemPos_(itemPos)
        {
            Y_ASSERT(Searcher_);
            Y_ASSERT(ItemPos_ < Searcher_->ItemCount());
        }

        ui32 DocumentId() const {
            return Searcher_->DocumentId_[ItemPos_];
        }

        ui32 ItemId() const {
            return Searcher_->ItemId_[ItemPos_];
        }

        const TFeatureType* Item() const {
            return Searcher_->Item_ + ItemPos_ * Searcher_->AlignedDimension_;
        }

        size_t Dimension() const {
            return Searcher_->Dimension();
        }

    private:
        const TSearcher* Searcher_ = nullptr;
        ui32 ItemPos_ = 0;
    };

}
