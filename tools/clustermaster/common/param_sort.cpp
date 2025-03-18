#include "param_sort.h"

namespace {
    TStringBuf TrimTrailingZeros(TStringBuf a) {
        while (a.at(0) == '0') {
            a = a.substr(1);
        }
        return a;
    }

    size_t FrontDigitCount(TStringBuf a) {
        ui32 i = 0;
        while (i < a.size() && isdigit(a.at(i))) {
            ++i;
        }
        return i;
    }
}

bool NiceLessImpl(const TStringBuf& a0, const TStringBuf& b0) {
    TStringBuf a = a0;
    TStringBuf b = b0;

    if (a.empty() || b.empty()) {
        return (!a.empty()) < (!b.empty());
    }

    if (isdigit(a.at(0)) && isdigit(b.at(0))) {
        a = TrimTrailingZeros(a);
        b = TrimTrailingZeros(b);
        size_t ad = FrontDigitCount(a);
        size_t bd = FrontDigitCount(b);
        if (ad != bd) {
            return ad < bd;
        }

        for (size_t i = 0; i < ad; ++i) {
            if (a.at(i) != b.at(i)) {
                return a.at(i) < b.at(i);
            }
        }

        return NiceLessImpl(a.substr(ad), b.substr(bd));
    } else {
        if (a.at(0) != b.at(0)) {
            return a.at(0) < b.at(0);
        }

        return NiceLessImpl(a.substr(1), b.substr(1));
    }
}
