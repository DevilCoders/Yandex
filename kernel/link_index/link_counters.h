#pragma once

#include <util/system/defaults.h>

struct TLinkCounters {
    ui32 Internal;
    ui32 External;

    TLinkCounters() {
        Reset();
    }

    void Reset() {
        Internal = 0;
        External = 0;
    }
};
