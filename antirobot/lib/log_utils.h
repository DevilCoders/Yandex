#pragma once

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>

namespace NAntiRobot {
    //TRASH
    void AdvancedUrlUnescape(char* to, const TStringBuf& from);

    TInstant GetRequestTime(const TStringBuf& timeStr);
}
