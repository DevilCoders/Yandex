#pragma once

#include "sent.h"

#include <kernel/segmentator/structs/structs.h>

#include <util/generic/hash_set.h>

class TArchiveMarkupZones;

namespace NSnippets::NSegments {
        using  TSegmentCIt = NSegm::TSegmentSpans::const_iterator;

        class TSegmentsInfo {
        private:
            NSegm::TSegmentSpans Segments;
            THashSet<ui16> MainContentSentBegins;
            THashSet<ui16> MainHeaderSentBegins;
            THashMap<int, int> Sent2Segment;
        public:
            explicit TSegmentsInfo(const TArchiveMarkupZones& zones);
            inline bool HasData() const
            {
                return Segments.ysize() > 0;
            }
            TSegmentCIt GetArchiveSegment(const int sentId) const
            {
                THashMap<int, int>::const_iterator seg = Sent2Segment.find(sentId);
                if (seg == Sent2Segment.end()) {
                    return Segments.end();
                }
                return Segments.begin() + seg->second;
            }
            TSegmentCIt GetArchiveSegment(const TArchiveSent& sent) const
            {
                if (sent.SourceArc != ARC_TEXT) {
                    return Segments.end();
                }
                return GetArchiveSegment(sent.SentId);
            }
            bool IsValid(TSegmentCIt segment) const
            {
                return Segments.begin() <= segment && segment < Segments.end();
            }
            TSegmentCIt SegmentsBegin() const
            {
                return Segments.begin();
            }
            TSegmentCIt SegmentsEnd() const
            {
                return Segments.end();
            }
            NSegm::ESegmentType GetType(TSegmentCIt segment) const
            {
                using namespace NSegm;
                if (!IsValid(segment))
                    return STP_NONE;
                return ESegmentType(segment->Type);
            }
            bool IsMainContent(TSegmentCIt segment) const
            {
                if (IsValid(segment)) {
                    return MainContentSentBegins.find(segment->Begin.Sent()) != MainContentSentBegins.end();
                }
                return false;
            }
            bool IsMainHeader(TSegmentCIt segment) const
            {
                if (IsValid(segment)) {
                    return MainHeaderSentBegins.find(segment->Begin.Sent()) != MainHeaderSentBegins.end();
                }
                return false;
            }
            bool IsMain(TSegmentCIt segment) const
            {
                return IsMainContent(segment) || IsMainHeader(segment);
            }
        };
}
