#pragma once

#include <util/system/defaults.h>

struct THitGroup {
    i32 Docids[8];
    ui32 Postings[8];
    bool SameDoc;
    int Ptr;
    int End;
    int Count;
    const ui8 *OldPostings;

    void SetSize(int count) {
        Count = count;
    }

    void Reset() {
        Ptr = 0;
        End = 0;
        OldPostings = nullptr;
#if defined(WITH_VALGRIND) || defined(_msan_enabled_)
        // indeed, SSE code used in DecoderFallback::SmallSkipToDoc
        // that reads uninitialized memory, always gives predictable results.
        // so we can zero memory only for memcheck/msan
        memset(Docids, 0, sizeof(Docids));
#endif
    }

    THitGroup() {
        Reset();
    }
};

void DecodePostings(THitGroup &hitGroup);
ui8 *Compress(const SUPERLONG *from, ui8 *coded, ui8 num);
const char *DecompressAll(THitGroup &hitGroup, const char *coded);
bool DecompressSkip(SUPERLONG &current, THitGroup &hitGroup, const char *&coded, i32 docSkip);
