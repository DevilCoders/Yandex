#pragma once

#include <util/system/types.h>
#include <util/generic/ylimits.h>

namespace NTextMachine {
    using TBlockId = ui16;
    using TQueryId = ui16;
    using TQueryWordId = ui16;
    using TWordFormId = ui16;
    using TWordLemmaId = ui16;

    using TStreamWordId = ui32;
    using TBreakId = ui16;
    using TBreakWordId = ui16;
    using TLowLevelWordFormId = ui16;
    using TStreamIndex = ui16;
    using THitRelevance = ui16;

    struct TAbsentValue {
        static constexpr TStreamWordId StreamWordCount = 0;
        static constexpr TBreakId StreamBreakCount = 0;
        static constexpr float StreamMaxValue = 0.0f;

        static constexpr TStreamIndex StreamIndex = Max<TStreamIndex>();
        static constexpr TBreakId BreakId = Max<TBreakId>();
        static constexpr TStreamWordId StreamWordPos = Max<TStreamWordId>();
        static constexpr TBreakWordId BreakWordCount = 0;
        static constexpr float AnnotationValue = 0.0f;

        static constexpr TBreakWordId BreakWordPos = Max<TBreakWordId>();

        static constexpr TBlockId BlockId = Max<TBlockId>();
        static constexpr TQueryId QueryId = Max<TQueryId>();
        static constexpr TQueryWordId QueryWordId = Max<TQueryWordId>();
        static constexpr TWordFormId WordFormId = Max<TWordFormId>();

        static constexpr float HitWeight = 0.0f;
        static constexpr THitRelevance HitRelevance = 0;
        static constexpr TWordLemmaId HitLemmaId = Max<TWordLemmaId>();
        static constexpr TWordFormId HitFormId = Max<TWordFormId>();
        static constexpr TLowLevelWordFormId HitLowLevelFormId = Max<TLowLevelWordFormId>();
    };
} // NTextMachine
