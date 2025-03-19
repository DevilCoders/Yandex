#pragma once

#include <kernel/reqbundle_iterator/reqbundle_iterator_fwd.h>

#include <util/generic/array_ref.h>

namespace NTrOverReqBundleIterator {
    class TReqBundlePosBufHitsProvider: public IReqBundleHitsProvider {
    public:
        TReqBundlePosBufHitsProvider(
            ui32 docId,
            TReqBundleIterator* iterator,
            const NReqBundleIterator::TPosBuf* positions)
            : DocId_(docId)
            , Iterator_(iterator)
            , Positions_(positions)
        {
        }

        size_t GetDocumentPositionsPartial(
            NReqBundleIterator::TPosition* res,
            size_t bufSize,
            TDynBitMap* goodBlocks) override;

        void GetRichTreeFormIds(
            NReqBundleIterator::TPosition* positions,
            size_t count,
            ui16* res) const override;

        void InitForDoc(ui32 docId) override;
        ui32 GetCurrentDoc() const override {
            return DocId_;
        }

    private:
        ui32 DocId_;
        TReqBundleIterator* Iterator_ = nullptr;
        const NReqBundleIterator::TPosBuf* Positions_ = nullptr;
        size_t PositionsPtr_ = 0;
    };

} // namespace NTrOverReqBundleIterator
