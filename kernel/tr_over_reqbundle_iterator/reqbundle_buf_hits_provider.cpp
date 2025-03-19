#include "reqbundle_buf_hits_provider.h"

#include <kernel/reqbundle_iterator/pos_buf.h>
#include <kernel/reqbundle_iterator/position.h>
#include <kernel/reqbundle_iterator/reqbundle_iterator.h>

namespace {
    inline size_t CopyHits(
        size_t& positionsPtr,
        size_t bufSize,
        const NReqBundleIterator::TPosBuf* positions,
        TDynBitMap* goodBlocks,
        NReqBundleIterator::TPosition* res)
    {
        if (!positions) {
            return 0;
        }
        size_t count = 0;
        while (positionsPtr < positions->Count && count < bufSize) {
            const NReqBundleIterator::TPosition& position = positions->Pos[positionsPtr++];
            const ui32 blockId = position.BlockId();
            if (!goodBlocks || goodBlocks->Get(blockId)) {
                res[count++] = position;
            }
        }
        return count;
    }
}

namespace NTrOverReqBundleIterator {

    void TReqBundlePosBufHitsProvider::InitForDoc(ui32 docId) {
        Y_ASSERT(docId == DocId_);
        if (Iterator_->GetCurrentDoc() != docId) {
            // does not happen in production under normal conditions,
            // but there are a lot of weird flags, so be safe
            Y_ASSERT(false);
            Iterator_->InitForDoc(docId);
            PositionsPtr_ = Positions_->Count;
        }
    }

    size_t TReqBundlePosBufHitsProvider::GetDocumentPositionsPartial(
        NReqBundleIterator::TPosition* res,
        size_t bufSize,
        TDynBitMap* goodBlocks)
    {
        size_t copied = ::CopyHits(PositionsPtr_, bufSize, Positions_, goodBlocks, res);
        if (copied < bufSize) {
            copied += Iterator_->GetDocumentPositionsPartial(res + copied, bufSize - copied, goodBlocks);
        }
        return copied;
    }

    void TReqBundlePosBufHitsProvider::GetRichTreeFormIds(
        NReqBundleIterator::TPosition* positions,
        size_t count,
        ui16* res) const
    {
        Iterator_->GetRichTreeFormIds(positions, count, res);
    }

} // namespace NTrOverReqBundleIterator
