#pragma once

#include "rule.h"

namespace NAntiRobot {
    namespace NMatchRequest {
       TRule ParseRule(const TString& ruleString);
    }
}
