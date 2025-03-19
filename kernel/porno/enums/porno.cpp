#include "porno.h"

#include <util/stream/output.h>
#include <util/string/cast.h>

template <>
void Out<EPornoFilterMode>(IOutputStream& o, const EPornoFilterMode v) {
    switch (v) {
        case PFM_OFF:
            o.Write('0');
            break;
        case PFM_MODERATE:
            o.Write('1');
            break;
        case PFM_STRICT:
            o.Write('2');
            break;
    }
}

template <>
bool TryFromStringImpl<EPornoFilterMode>(const char* x, size_t len, EPornoFilterMode& v) {
    const TStringBuf s{x, len};
    if (s == "0") {
        v = PFM_OFF;
        return true;
    } else if (s == "1") {
        v = PFM_MODERATE;
        return true;
    } else if (s == "2") {
        v = PFM_STRICT;
        return true;
    }
    return false;
}

template <>
EPornoFilterMode FromStringImpl<EPornoFilterMode>(const char* x, size_t len) {
    EPornoFilterMode v;
    if (TryFromStringImpl(x, len, v)) {
        return v;
    }
    ythrow TFromStringException{};
}
