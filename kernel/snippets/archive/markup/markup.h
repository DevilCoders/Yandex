#pragma once

#include <kernel/snippets/iface/archive/enums.h>

#include <kernel/snippets/hits/ctx.h>

#include <kernel/tarc/iface/tarcface.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NSnippets {
    namespace NSegments {
        class TSegmentsInfo;
    };

    class TArchiveMarkup {
    private:
        struct {
            THolder<TArchiveMarkupZones> Markup;
            TVector<ui16> FilteredHits;
        } Arc[ARC_COUNT];
        THitsInfoPtr Hits;
        THolder<NSegments::TSegmentsInfo> Segments;
    public:
        TArchiveMarkup();
        ~TArchiveMarkup();
        const TArchiveMarkupZones& GetArcMarkup(EARC arc) const;
        void SetArcMarkup(EARC arc, const void* markupInfo, size_t markupInfoLen);
        const NSegments::TSegmentsInfo* GetSegments() const;
        void ResetSegments(NSegments::TSegmentsInfo* segments);
        void SetHitsInfo(const THitsInfoPtr& hits);
        void FilterHits(EARC arc, ui32 hitsTopLen, bool allHits);
        const TVector<ui16>& GetFilteredHits(EARC arc) const;
    };
}
