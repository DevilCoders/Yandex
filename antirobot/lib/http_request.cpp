#include "http_request.h"

#include <util/network/endpoint.h>
#include <util/string/subst.h>

namespace NAntiRobot {

/* THttpRequest */

THttpRequest::THttpRequest(const TString& method, const TString& host, const TString& path)
    : Method(method)
    , Path(path)
{
    SetHost(host);
}

THttpRequest& THttpRequest::AddHeader(const TString& name, const TString& value) {
    Headers.AddHeader(THttpInputHeader(name, value));
    return *this;
}

THttpRequest& THttpRequest::AddCgiParam(const TString& name, const TString& value) {
    Cgi.InsertUnescaped(name, value);
    return *this;
}

THttpRequest& THttpRequest::AddCgiParameters(const TCgiParameters& params) {
    Cgi.insert(params.begin(), params.end());
    return *this;
}

THttpRequest& THttpRequest::SetContent(TString content) {
    Content = std::move(content);
    Headers.AddOrReplaceHeader(THttpInputHeader("Content-Length", ::ToString(Content.length())));
    return *this;
}

THttpRequest& THttpRequest::SetHost(const TString& host) {
    Headers.AddOrReplaceHeader(THttpInputHeader("Host", host));
    return *this;
}

TString THttpRequest::CreateUrl(const TNetworkAddress& addr, const TString& protocol) const {
    TEndpoint endpoint(new NAddr::TAddrInfo(&*addr.Begin()));
    return TString::Join(protocol, "://", endpoint.IpToString(), ":", ToString(endpoint.Port()), Path, "?", Cgi.Print());
}

TString THttpRequest::GetPathAndQuery() const {
    TStringStream out;
    out << Path;
    if (!Cgi.empty()) {
        out << '?' << Cgi.Print();
    }

    return out.Str();
}

IOutputStream& operator << (IOutputStream& os, const THttpRequest& req) {
    static const TStringBuf CRLF("\r\n");

    os << req.Method << " " << req.Path;
    if (!req.Cgi.empty()) {
        os << '?' << req.Cgi.Print();
    }
    os << " HTTP/1.1" << CRLF;
    req.Headers.OutTo(&os);
    os << CRLF;
    if (!req.Content.empty()) {
        os << req.Content;
    }
    return os;
}

THttpRequest HttpGet(const TString& host, const TString& path) {
    return THttpRequest("GET", host, path);
}

THttpRequest HttpPost(const TString& host, const TString& path) {
    return THttpRequest("POST", host, path);
}

TString THttpRequest::ToLogString() const {
    TString s = ToString(*this);
    SubstGlobal(s, "\r", "\\r");
    SubstGlobal(s, "\n", "\\n");
    return s;
}

}
