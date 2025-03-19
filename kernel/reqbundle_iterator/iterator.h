#pragma once

#include "position.h"
#include "reqbundle_hits_provider.h"
#include "reqbundle_iterator_fwd.h"

#include <kernel/doom/search_fetcher/search_fetcher.h>

#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/noncopyable.h>

namespace NReqBundleIterator {
    class TRBIterator
        : public TNonCopyable,
          public IRBHitsProvider
    {
    private:
        friend class TRBIteratorsFactory;

        class TImpl;
        THolder<TImpl> Impl;

        // These fields are moved here
        // from Impl, because fast
        // access (and inlined GetRichTreeFormId)
        // are critical for performance
        TVector<ui16> RichTreeFormIds;
        TVector<ui32> RichTreeBlockOffsets;

        TImpl& GetImpl();

    public:
        TRBIterator();
        ~TRBIterator();

        void AnnounceDocIds(TConstArrayRef<ui32> docIds);
        void PrefetchDocs(const TVector<ui32>& docIds);

        void PreLoadDoc(ui32, const NDoom::TSearchDocLoader&);
        void AdviseDocIds(TConstArrayRef<ui32>, std::function<void(ui32)> consumer);

        void InitForDoc(ui32 docId) override;
        ui32 GetCurrentDoc() const override;

        // if goodBlocks is set, hits for blocks with !goodBlocks[blockIndex]
        // are silently discarded and cannot be retrieved later
        size_t GetDocumentPositionsPartial(
            TPosition* res,
            size_t bufSize,
            TDynBitMap* goodBlocks) override;

        bool LookupNextDoc(ui32& docId);
        bool LookupNextDoc(ui32& docId, ui32 maxDecay);

        ui16 GetRichTreeFormId(
            ui32 blockId,
            ui32 lowLevelFormId) const;

        void GetRichTreeFormIds(
            TPosition* positions,
            size_t count,
            ui16* res) const override
        {
            if (count == 0) {
                return;
            }

            Y_ASSERT(positions);
            Y_ASSERT(res);
            for (size_t i = 0; i < count; ++i) {
                const TPosition& position = positions[i];
                res[i] = GetRichTreeFormId(position.BlockId(), position.LowLevelFormId());
            }
        }

        ui32 GetNumBlocks() const {
            return RichTreeBlockOffsets.size();
        }
    };

    inline ui16 TRBIterator::GetRichTreeFormId(
        ui32 blockId,
        ui32 lowLevelFormId) const
    {
        const size_t index = RichTreeBlockOffsets[blockId] + lowLevelFormId;
        return RichTreeFormIds[index];
    }
} // NReqBundleIterator
