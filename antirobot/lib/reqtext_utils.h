#pragma once

#include "ar_utils.h"

#include <library/cpp/cgiparam/cgiparam.h>

namespace NAntiRobot {
    TString ExtractReqTextFromCgi(const TCgiParameters& cgiParams);
}
