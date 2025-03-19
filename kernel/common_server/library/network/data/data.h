#pragma once

#include <util/generic/strbuf.h>

#include <utility>

class TBaseServerRequestData;
class TCgiParameters;

namespace NUtil {
    TStringBuf GetClientIp(const TBaseServerRequestData& rd);
    TStringBuf GetClientIp(const TBaseServerRequestData* rd);
    TStringBuf GetReqId(const TBaseServerRequestData& rd, const TCgiParameters& cgi);
    std::pair<TString, ui16> GetSlotFromFqdn(TStringBuf fqdn);
}
