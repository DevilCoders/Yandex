#pragma once
#include "request_params.h"

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAntiRobot {

bool NeedJSONP(const TCgiParameters& cgi);
TString ToJSONP(const TCgiParameters& cgi, const TStringBuf& json);

bool IsCorrectJsonpCallback(const TCgiParameters& cgi);

}
