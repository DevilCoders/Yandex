#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/fwd.h>

namespace NFacts {
    void FillUrlAndGreenUrl(const TStringBuf& rawUrl, NSc::TValue& serpData, bool isTouch = false);
}
