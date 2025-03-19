#pragma once

#include <library/cpp/packedtypes/packedfloat.h>
#include <util/system/hi_lo.h>

inline
float GetFloatFromSf16(ui32 value) {
    float f = 0;
    Hi16(f) = (ui16)value;
    return f;
}

inline
ui16 GetSf16FromFloat(float f) {
    return Hi16(f);
}

inline
float GetFloatFromUf16(ui32 value) {
    return (float)uf16(ui16(value));
}

inline
ui16 GetUf16FromFloat(float f) {
    return uf16::New(f).val;
}
