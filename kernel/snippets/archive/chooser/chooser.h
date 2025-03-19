#pragma once

#include <kernel/snippets/archive/view/view.h>

#include <util/generic/vector.h>

namespace NSnippets {
    class TFirstAndHitSentsViewer;
    class TArchiveView;
    class TConfig;
    class TSentsOrder;
    namespace NSegments {
        class TSegmentsInfo;
    }

    class TSentFilter {
    private:
        TVector<bool> SentIsFlagged;

        bool IsValidSent(int arcId) const {
            return arcId >= 0 && arcId < 0xFFFF;
        }

    public:
        TSentFilter(int nSents = 0) {
            SentIsFlagged.resize(nSents, false);
        }

        void Mark(int arcId) {
            if (!IsValidSent(arcId))
                return;
            if (arcId >= SentIsFlagged.ysize())
                SentIsFlagged.resize(arcId + 1, false);
            SentIsFlagged[arcId] = true;
        }

        bool IsMarked(int arcId) const {
            return (IsValidSent(arcId) && arcId < SentIsFlagged.ysize()) ? SentIsFlagged[arcId] : false;
        }
    };

    struct TChooserContext {
        TArchiveView Title;
        TArchiveView NearHit;
        TArchiveView Hit;
    };

    void ChooseArchiveSents(const TConfig& cfg, const TChooserContext& ctx, const NSegments::TSegmentsInfo* seg, TArchiveView& res, const TSentFilter& excludeFilter = TSentFilter());
}
