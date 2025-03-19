#include "segments.h"

#include <kernel/tarc/iface/tarcface.h>
#include <kernel/tarc/markup_zones/unpackers.h>

namespace NSnippets {
    namespace NSegments {
        TSegmentsInfo::TSegmentsInfo(const TArchiveMarkupZones& zones) {
            for (const TArchiveZoneSpan& span : zones.GetZone(AZ_MAIN_CONTENT).Spans) {
                MainContentSentBegins.insert(span.SentBeg);
            }
            for (const TArchiveZoneSpan& span : zones.GetZone(AZ_MAIN_HEADER).Spans) {
                MainHeaderSentBegins.insert(span.SentBeg);
            }

            Segments = NSegm::GetSegmentsFromArchive(zones);

            int i = 0;
            int j = 0;
            while (i < Segments.ysize()) {
                if (j < Segments[i].Begin.Sent()) {
                    ++j;
                    continue;
                }
                if (Segments[i].ContainsSent(j)) {
                    Sent2Segment[j] = i;
                    ++j;
                    continue;
                }
                if (Segments[i].Begin.Sent() < j) {
                    ++i;
                    continue;
                }
                Y_ASSERT(0);
            }
        }

    }
}
