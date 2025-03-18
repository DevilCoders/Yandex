#pragma once

#include "request_params.h"

#include <util/generic/string.h>


namespace NAntiRobot {


bool CheckAutoRuTamper(
    const THeadersMap& headers,
    const TCgiParameters& cgiParams,
    TStringBuf salt
);


} // namespace NAntiRobot
