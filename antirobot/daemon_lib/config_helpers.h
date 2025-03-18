#pragma once

#include <library/cpp/yconf/conf.h>

namespace NAntiRobot {
    DECLARE_CONFIG(TAntirobotConfStrings)

    // omnivorous config
    inline bool TAntirobotConfStrings::OnBeginSection(Section& sec) {
        sec.Cookie = new AnyDirectives;
        sec.Owner = true;
        return true;
    }
}
