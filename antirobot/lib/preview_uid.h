#pragma once

#include <util/string/util.h>

namespace NAntiRobot {

enum EPreviewAgentType {
    UNKNOWN = 0,
    OTHER = 1,
};

EPreviewAgentType GetPreviewAgentType(TStringBuf userAgent);

}
