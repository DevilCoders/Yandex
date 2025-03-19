#pragma once

#include <util/system/defaults.h>

struct TInterval {
    ui32 Begin;
    ui32 End;

    TInterval();
    TInterval(ui32 begin, ui32 end);
    explicit TInterval(ui64 packed);

    ui64 Pack() const;
};
