#include "dt_breaksegment.h"

#include <util/charset/unidata.h>

namespace NDT {

using namespace NIndexerCore;

void TDTSplitSegmenter::DoSegmentation(ISegmentCallback& cb, const NIndexerCore::TDirectTextEntry2* entries,
    size_t from, size_t to, const TLangMask& langs) const {

    size_t i = FindNextBreak(entries, from, to);
    while (i < to) {
        cb.OnSegment(entries, from, i, langs);
        from = i;
        i = FindNextBreak(entries, from, to);
    }
    if (from < to) {
        cb.OnSegment(entries, from, to, langs);
    }
}

size_t TDTSentSegmenter::FindNextBreak(const TDirectTextEntry2* entries,
    size_t from, size_t to) const {

    if (from >= to)
        return to;

    if (!entries[from].Token.IsInited()) {
        // Don't break after first token if it is dummy.
        ++from;
    }

    for (size_t i = from; i < to; ++i) {
        const TDirectTextEntry2& entry = entries[i];
        for (size_t spaceIndex = 0; spaceIndex < entry.SpaceCount; ++spaceIndex) {
            const TDirectTextSpace& space = entry.Spaces[spaceIndex];
            if (0 != (BreakMask & space.Type)) {
                return i + 1; // Stop after current entry
            }
        }
    }
    return to; // Stop at end
}

size_t TDTExtBreakSegmenter::FindNextBreak(const TDirectTextEntry2* entries,
    size_t from, size_t to) const {

    const TPosting nextBreak = GetBreakAfter(entries[from].Posting);
    for (size_t i = from + 1; i < to; ++i) {
        const TDirectTextEntry2& entry = entries[i];
        if (entry.Posting >= nextBreak) {
            return i; // Stop at current entry
        }
    }
    return to; // Stop at end
}

void TDTDelimSegmenter::SetDelims(const TUtf16String& delims) {
    for (size_t i = 0; i < delims.size(); ++i) {
        Y_ASSERT(!IsHighSurrogate(delims.at(i)) && !IsLowSurrogate(delims.at(i)));
        Delims.insert(delims.at(i));
    }
}

size_t TDTDelimSegmenter::FindNextBreak(const TDirectTextEntry2* entries,
    size_t from, size_t to) const {

    if (from >= to)
        return to;

    if (!entries[from].Token.IsInited()) {
        // Don't break after first token if it is dummy.
        ++from;
    }

    for (size_t i = from; i < to; ++i) {
        const TDirectTextEntry2& entry = entries[i];
        for (size_t spaceIndex = 0; spaceIndex < entry.SpaceCount; ++spaceIndex) {
            const TDirectTextSpace& space = entry.Spaces[spaceIndex];
            for (size_t s = 0; s < space.Length; ++s) {
                if (Delims.Has(space.Space[s])) {
                    return i + 1; // Stop after current entry
                }
            }
        }
    }
    return to; // Stop at end
}

}
