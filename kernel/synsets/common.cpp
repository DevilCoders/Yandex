#include <util/system/yassert.h>

#include "common.h"

TInterval::TInterval()
    : Begin(0)
    , End(0)
{
}

TInterval::TInterval(ui32 begin, ui32 end)
    : Begin(begin)
    , End(end)
{
}

TInterval::TInterval(ui64 packed) {
    static const ui64 MASK = (((ui64)1) << 32) - 1;
    Begin = packed >> 32;
    End = packed & MASK;
    Y_ASSERT( Pack() == packed );
}

ui64 TInterval::Pack() const {
    return (((ui64)Begin) << 32) + ui64(End);
}
