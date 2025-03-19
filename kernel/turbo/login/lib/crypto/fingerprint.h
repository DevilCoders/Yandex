#pragma once

#include <util/generic/string.h>

namespace NTurboLogin {
    ui64 GetFingerPrint(const TString& userAgent, const TString& ip);
}
