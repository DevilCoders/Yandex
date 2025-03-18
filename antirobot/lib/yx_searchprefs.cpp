#include "yx_searchprefs.h"

#include <library/cpp/http/cookies/cookies.h>
#include <library/cpp/string_utils/scan/scan.h>

#include <util/string/cast.h>
#include <util/string/strip.h>

using namespace NAntiRobot;

namespace {
    struct TScanner {
        TYxSearchPrefs* P;

        inline void operator() (TStringBuf key, TStringBuf value) {
            key = StripString(key);
            value = StripString(value);

            if (key == "numdoc"sv) {
                P->NumDoc = FromStringWithDefault<ui32>(value);
            } else if (key == "lr"sv) {
                P->Lr = FromStringWithDefault<i32>(value);
            }
        }
    };
}

void TYxSearchPrefs::Init(const THttpCookies& cookies) {
    TScanner s = {this};
    ScanKeyValue<false, ',', ':'>(cookies.Get(TStringBuf("YX_SEARCHPREFS")), s);
}
