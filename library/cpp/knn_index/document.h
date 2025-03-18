#pragma once

#include "constants.h"

#include <util/system/align.h>
#include <util/system/types.h>

namespace NKNNIndex {
    /**
     * Class for document-in-index representation.
     * Provides DocumentId and Items(feature vectors of this documents with ids)
     */
    template <class Searcher>
    class TDocument {
    public:
        using TSearcher = Searcher;
        using TFeatureType = typename TSearcher::TFeatureType;

        explicit TDocument(const TSearcher* searcher, ui32 documentId, bool exists)
            : Searcher_(searcher)
            , DocumentId_(documentId)
        {
            Y_ASSERT(Searcher_);
            const ui32* documentOffset = Searcher_->DocumentOffset_;
            if (exists) {
                ItemCount_ = documentOffset[documentId + 1] - documentOffset[documentId];
                ItemPos_ = Searcher_->DocumentItem_ + documentOffset[documentId];
            }
        }

        ui32 DocumentId() const {
            return DocumentId_;
        }

        ui32 ItemId(size_t index) const {
            Y_ASSERT(index < ItemCount());
            return Searcher_->ItemId_[ItemPos_[index]];
        }

        const TFeatureType* Item(size_t index) const {
            Y_ASSERT(index < ItemCount());
            return Searcher_->Item_ + ItemPos_[index] * Searcher_->AlignedDimension_;
        }

        size_t ItemCount() const {
            return ItemCount_;
        }

        size_t Dimension() const {
            Y_ASSERT(Searcher_);
            return Searcher_->Dimension();
        }

    private:
        const TSearcher* Searcher_ = nullptr;
        ui32 DocumentId_ = InvalidDocumentId;
        size_t ItemCount_ = 0;
        const ui32* ItemPos_ = nullptr;
    };

}
