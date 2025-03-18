#pragma once

#include "page_segment.h"

#include <util/generic/vector.h>

namespace NSegmentator {
    struct TMetaSegmentatorCfg {
        bool NeedDMD4MainContent = false;
        bool NeedDHA4MainContent = false;
    };

    class TMetaSegmentator {
    public:
        TMetaSegmentator(const TVector<TPageSegment>& pageSegments, const TMetaSegmentatorCfg& cfg);
        void FillMetaSegments();
        const TVector<TPageSegment>& GetMainContent() const;

    private:
        void FillMainContent();
        void ResetAllMetaSegments();

    private:
        const TVector<TPageSegment>& PageSegments;
        TVector<TPageSegment> MainContent;
        const TMetaSegmentatorCfg& Cfg;
    };
}
