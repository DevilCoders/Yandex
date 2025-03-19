#include "ansi_escape.h"

#include <util/generic/singleton.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NReMorph {

namespace {

static const char* CSI = "\033[";
static const size_t COLORS = 8;

class TSgrColorEscapeCodes {
private:
    TVector<TString> EscapeCodes;

public:
    TSgrColorEscapeCodes()
        : EscapeCodes(COLORS * 2 + 1, TString(CSI))
    {
        for (size_t c = 0; c < COLORS; ++c) {
            TString color = ::ToString(30 + c);
            EscapeCodes[c * 2].append(color);
            EscapeCodes[c * 2].append("m");
            EscapeCodes[c * 2 + 1].append("1;");
            EscapeCodes[c * 2 + 1].append(color);
            EscapeCodes[c * 2 + 1].append("m");
        }
        EscapeCodes[COLORS * 2].append("0m");
    }

    inline TStringBuf Color(size_t color, bool bold) const {
        return EscapeCodes[color * 2 + bold];
    }

    inline TStringBuf ResetColor() const {
        return EscapeCodes[COLORS * 2];
    }
};

}

TStringBuf AnsiSgrColor(EColorCode color, bool bold) {
    return Default<TSgrColorEscapeCodes>().Color(static_cast<size_t>(color), bold);
}

TStringBuf AnsiSgrColorReset() {
    return Default<TSgrColorEscapeCodes>().ResetColor();
}

} // NReMorph
