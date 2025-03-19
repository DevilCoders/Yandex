#pragma once

#include <util/generic/bitmap.h>
#include <util/generic/ylimits.h>
#include <util/system/types.h>

namespace NReqBundleIterator {

    class TPosition;

    class IRBHitsProvider {
    public:
        virtual ~IRBHitsProvider() = default;

        virtual void InitForDoc(ui32 docId) = 0;
        virtual ui32 GetCurrentDoc() const = 0;

        // if goodBlocks is given, hits for blocks with !(*goodBlocks)[blockIndex]
        // are silently discarded and cannot be retrieved later
        virtual size_t GetDocumentPositionsPartial(
            TPosition* res,
            size_t bufSize,
            TDynBitMap* goodBlocks = nullptr) = 0;

        size_t GetAllDocumentPositions(
            ui32 docId,
            TPosition* res,
            size_t bufSize)
        {
            InitForDoc(docId);
            return GetDocumentPositionsPartial(res, bufSize, nullptr);
        }

        virtual void GetRichTreeFormIds(
            TPosition* positions,
            size_t count,
            ui16* res) const = 0;
    };

} // namespace NReqBundleIterator
