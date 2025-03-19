#pragma once

#include <kernel/tarc/iface/tarcface.h>

#include <util/generic/vector.h>
#include <util/generic/strbuf.h>

namespace NSnippets {
    class TSingleSnip;

    class TZoneWords {
    public:
        TArchiveZoneSpan Span;
        int FirstWordId;
        int LastWordId;
        TZoneWords(const TArchiveZoneSpan& span, int lBorder, int rBorder)
            : Span(span)
            , FirstWordId(lBorder)
            , LastWordId(rBorder)
        {
        }
    };

    void GetZones(const TSingleSnip& snip, EArchiveZone zName, TVector<TZoneWords>& borders, bool strictlyInnerZones, bool fuzzyWordCheck = false);
}
