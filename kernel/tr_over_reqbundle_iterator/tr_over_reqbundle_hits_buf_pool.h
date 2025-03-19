#pragma once

#include <kernel/reqbundle_iterator/position.h>

#include <ysite/yandex/posfilter/dochitsbuf.h>

namespace NTrOverReqBundleIterator {
    class TTrOverReqBundleHitsBufPool {
    public:
        enum {
            MaxHits = 3 * (ui32(1) << 11),
            MaxDocHits = 3 * (ui32(1) << 9)
        };

        void Reset() {
            ReqBundleHitsPtr = 0;
            TrHitsPtr = 0;
        }

        NReqBundleIterator::TPosition* GetReqBundleHitPos() {
            return &ReqBundlePositions[ReqBundleHitsPtr];
        }

        ui16* GetRichTreeFormPos() {
            return &RichTreeForms[ReqBundleHitsPtr];
        }

        TFullPositionEx* GetTrHitPos() {
            return &TrPositions[TrHitsPtr];
        }

        THitInfo* GetHitInfoPos() {
            return &HitsInfo[TrHitsPtr];
        }

        void AdvanceReqBundleHits(size_t hitsCount) {
            Y_ASSERT(hitsCount <= MaxHits - ReqBundleHitsPtr);
            ReqBundleHitsPtr += hitsCount;
        }

        void AdvanceTrHits(size_t hitsCount) {
            Y_ASSERT(hitsCount <= MaxHits - TrHitsPtr);
            TrHitsPtr += hitsCount;
        }

        size_t GetReqBundleHitsFreeSpaceSize() const {
            Y_ASSERT(ReqBundleHitsPtr <= MaxHits);
            return MaxHits - ReqBundleHitsPtr;
        }

        size_t GetTrHitsFreeSpaceSize() const {
            Y_ASSERT(TrHitsPtr <= MaxHits);
            return MaxHits - TrHitsPtr;
        }

    private:
        size_t ReqBundleHitsPtr = 0;
        size_t TrHitsPtr = 0;
        NReqBundleIterator::TPosition ReqBundlePositions[MaxHits];
        ui16 RichTreeForms[MaxHits];
        TFullPositionEx TrPositions[MaxHits];
        THitInfo HitsInfo[MaxHits];
    };
} // namespace NTrOverReqBundleIterator
