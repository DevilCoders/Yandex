#pragma once

#include <util/generic/strbuf.h>

class TCgiParameters;

namespace NAliceSearch {
    bool IsAliceMusicScenarioRequest(TStringBuf userRequest, const TCgiParameters& cgi);
} // namespace AliceSearch
