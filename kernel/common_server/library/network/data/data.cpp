#include "data.h"

#include <library/cpp/http/misc/httpreqdata.h>
#include <util/string/cast.h>

namespace {
    constexpr char HostnameDelimiter = '-';
    constexpr TStringBuf RuntimeCloudSuffix = ".search.yandex.net";
}

TStringBuf NUtil::GetClientIp(const TBaseServerRequestData& rd) {
    TStringBuf result;
    if (!result) {
        result = rd.HeaderInOrEmpty("X-Forwarded-For-Y");
    }
    if (!result) {
        result = rd.RemoteAddr();
    }
    return result;
}

TStringBuf NUtil::GetClientIp(const TBaseServerRequestData* rd) {
    if (rd) {
        return NUtil::GetClientIp(*rd);
    } else {
        return {};
    }
}

TStringBuf NUtil::GetReqId(const TBaseServerRequestData& rd, const TCgiParameters& cgi) {
    constexpr TStringBuf reqIdParameter = "reqid";
    if (const TString& fromCgi = cgi.Get(reqIdParameter)) {
        return fromCgi;
    }
    if (const auto* fromHeaders = rd.HeaderIn("X-Req-Id")) {
        return *fromHeaders;
    }
    return {};
}

std::pair<TString, ui16> NUtil::GetSlotFromFqdn(TStringBuf fqdn) {
    TString host;
    ui16 port = 0;

    auto p = fqdn.find(".gencfg-c.yandex.net");
    if (p != TStringBuf::npos) {
        TStringBuf hostname = fqdn.substr(0, p);
        TStringBuf dc = hostname.Before(HostnameDelimiter);
        TStringBuf id = hostname.After(HostnameDelimiter).Before(HostnameDelimiter);

        host = ToString(dc) + HostnameDelimiter + id + RuntimeCloudSuffix;
        TryFromString(hostname.RAfter(HostnameDelimiter), port);
    } else {
        host = fqdn.Before(':');
        TryFromString(fqdn.After(':'), port);
    }
    return { host, port };
}
