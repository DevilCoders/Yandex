#pragma once

#include "host_addr.h"

#include <library/cpp/http/io/headers.h>

#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NAntiRobot {

class THttpRequest {
public:
    THttpRequest(const TString& method, const TString& host, const TString& path);

    THttpRequest& AddHeader(const TString& name, const TString& value);
    THttpRequest& AddCgiParam(const TString& name, const TString& value);
    THttpRequest& AddCgiParameters(const TCgiParameters& params);

    THttpRequest& SetContent(TString content);
    THttpRequest& SetHost(const TString& host);

    TString CreateUrl(const TNetworkAddress& addr, const TString& protocol) const;

    TString GetHeadersAsString() const {
        TStringStream headers;
        Headers.OutTo(&headers);
        return headers.Str();
    }

    const TString& GetPath() const {
        return Path;
    }


    const TString& GetContent() const {
        return Content;
    }

    const TString& GetMethod() const {
        return Method;
    }

    TString GetPathAndQuery() const;

    TString ToLogString() const;

private:
    friend IOutputStream& operator << (IOutputStream& os, const THttpRequest& req);

    TString Method;
    TString Path;
    TCgiParameters Cgi;
    THttpHeaders Headers;
    TString Content;
};

THttpRequest HttpGet(const TString& host, const TString& path);
THttpRequest HttpPost(const TString& host, const TString& path);

inline THttpRequest HttpGet(const THostAddr& host, const TString& path) {
    return HttpGet(ToString(host), path);
}

inline THttpRequest HttpPost(const THostAddr& host, const TString& path) {
    return HttpPost(ToString(host), path);
}

}
