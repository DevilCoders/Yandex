#include "xmlsearchin.h"
#include "metahost.h"

#include <yweb/news/fetcher_lib/fetcher.h>
#include <library/cpp/http/io/stream.h>
#include <util/network/socket.h>
#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/subst.h>
#include <util/generic/hash.h>

namespace NSnippets {

    struct THost::TFetchImpl {
        TString Scheme;
        TString Host;
        ui16 Port = 0;
        TSimpleSharedPtr<TNetworkAddress> Addr;

        TFetchImpl(const TString& scheme, const TString& host, ui16 port)
          : Scheme(scheme)
          , Host(host)
          , Port(port)
        {
            Addr.Reset(new TNetworkAddress(host, port));
        }

        bool CanFetch() const {
            return Addr.Get() != nullptr;
        }

        TString Fetch(const TString& requestCgi, int follow) const;
    };

    THost::THost()
      : Scheme("http://")
    {
    }

    THost::THost(const THost& other)
      : Scheme(other.Scheme)
      , Host(other.Host)
      , Port(other.Port)
      , ExtraCgiParams(other.ExtraCgiParams)
      , FetchImpl(other.FetchImpl ? new TFetchImpl(*other.FetchImpl) : nullptr)
    {
    }

    THost& THost::operator=(const THost& other) {
        Scheme = other.Scheme;
        Host = other.Host;
        Port = other.Port;
        ExtraCgiParams = other.ExtraCgiParams;
        FetchImpl.Reset(other.FetchImpl ? new TFetchImpl(*other.FetchImpl) : nullptr);
        return *this;
    }

    THost::~THost() {
    }

    void THost::FillAddr() {
        try {
            FetchImpl.Reset(new TFetchImpl(Scheme, Host, Port));
        } catch (...) {
        }
    }

    void THost::RefillAddr() {
        FetchImpl.Reset(nullptr);
        FillAddr();
    }

    bool THost::CanFetch() const {
        return FetchImpl.Get() && FetchImpl->CanFetch();
    }

    static bool GetRedirect(const THttpHeaders& httpHeaders, TString& location) {
        for (THttpHeaders::TConstIterator it = httpHeaders.Begin(); it != httpHeaders.End(); ++it) {
            if (it->Name() == "Location") {
                location = it->Value();
                return true;
            }
        }
        return false;
    }

    TString THost::Fetch(const TString& requestCgi, int follow) const {
        if (!CanFetch()) {
            return TString();
        }
        return FetchImpl->Fetch(requestCgi, follow);
    }

    TString THost::TFetchImpl::Fetch(const TString& requestCgi, int follow) const {
        TStringBuf schemeStr = Scheme == "http2://" ? "http://" : Scheme;
        TString portStr = ((Port == 80 && schemeStr == "http://" || Port == 443 && schemeStr == "https://") ? "" : ":" + ToString(Port));
        if (Scheme == "https://") {
            TString url = TString(schemeStr);
            url += Host;
            url += portStr;
            url += requestCgi;
            NHttpFetcher::TRequestRef req = new NHttpFetcher::TRequest(url);
            req->UserAgent = "curl/7.19.7 (x86_64-pc-linux-gnu) libcurl/7.19.7 OpenSSL/0.9.8k zlib/1.2.3.3 libidn/1.15";
            NHttpFetcher::TResultRef fetchResult = NHttpFetcher::FetchNow(req);
            if (fetchResult->Code == 301 || fetchResult->Code == 302) {
                if (follow <= 0) {
                    return TString();
                }
                TString redir;
                if (GetRedirect(fetchResult->Headers, redir)) {
                    THost other;
                    TStringBuf otherPath;
                    TStringBuf otherCgi;
                    if (ParseUrl(redir, other, otherPath, otherCgi)) {
                        return other.Fetch(TString(otherPath.data(), otherPath.size()) + otherCgi, follow - 1);
                    }
                }
                return TString();
            } else if (fetchResult->Success()) {
                return fetchResult->DecodeData();
            } else {
                Cerr << "got bad code: " << fetchResult->Code << Endl;
                return TString();
            }
        } else {
            TSocket s(*Addr.Get());
            s.SetSocketTimeout(10, 0);
            s.SetNoDelay(true);
            {
                TSocketOutput so(s);
                THttpOutput o(&so);
                o.EnableCompression(true);
                o.EnableKeepAlive(true);
                TString r = "GET " + requestCgi;
                r += " HTTP/1.1\r\n";
                r += "Host: " + Host + portStr + "\r\n";
                r += "\r\n";
                o.Write(r.data(), r.size());
                o.Finish();
            }
            {
                TSocketInput si(s);
                THttpInput i(&si);
                unsigned httpCode = ParseHttpRetCode(i.FirstLine());
                if (follow > 0 && (httpCode == 301 || httpCode == 302)) { //bad stuff code, shouldn't be here most likely
                    TString redir;
                    if (GetRedirect(i.Headers(), redir)) {
                        THost other;
                        TStringBuf otherPath;
                        TStringBuf otherCgi;
                        if (ParseUrl(redir, other, otherPath, otherCgi)) {
                            return other.Fetch(TString(otherPath.data(), otherPath.size()) + otherCgi, follow - 1);
                        }
                    }
                    return TString();
                }
                return i.ReadAll();
            }
        }
    }

    TString THost::RawSearch(const TString& endpoint, const TString& text, const TString& lr, const TString& uil, const TString& groupby) const
    {
        if (!CanFetch()) {
            return {};
        }
        TCgiParameters cgi;
        cgi.InsertUnescaped("text", text);
        if (lr.size()) {
            cgi.InsertUnescaped("lr", lr);
        }
        if (groupby.size()) {
            cgi.InsertUnescaped("groupby", groupby);
        }
        if (uil.size()) {
            cgi.InsertUnescaped("l10n", uil);
            // for enabling uil="en" on ru domain
            cgi.InsertUnescaped("l10nismain", "1");
        }

        if (!!ExtraCgiParams) {
            cgi.ScanAddAll(ExtraCgiParams);
        }
        TString scgi = cgi.Print();
        SubstGlobal(scgi, ";", "%3B");

        TString response;
        try {
            response = Fetch(endpoint + "?" + scgi);
        } catch (...) {
            return {};
        }
        return response;
    }

    bool THost::XmlSearch(NXmlSearchIn::TRequest& res, const TString& text, const TString& source, const TString& xmltype, const TString& lr, const TString& groupby) const {
        if (!CanFetch()) {
            return false;
        }
        TCgiParameters cgi;
        cgi.InsertUnescaped("full-query", source);
        cgi.InsertUnescaped("text", text);
        if (xmltype.size()) {
            cgi.InsertUnescaped("type", xmltype);
        }
        if (lr.size()) {
            cgi.InsertUnescaped("lr", lr);
        }
        if (groupby.size()) {
            cgi.InsertUnescaped("groupby", groupby);
        }
        if (!!ExtraCgiParams) {
            cgi.ScanAddAll(ExtraCgiParams);
        }
        TString scgi = cgi.Print();
        SubstGlobal(scgi, ";", "%3B");

        TString xml;
        try {
            xml = Fetch("/search/xml?" + scgi);
        } catch (...) {
            return false;
        }
        return NSnippets::NXmlSearchIn::ParseRequest(res, xml);
    }

    struct TDomHosts::TImpl {
        typedef THashMap<TString, THost> THosts;
        THosts Hosts;
        const TString Scheme;
        const TString SearchDomainPrefix;
        ui16 Port = 0;
        const TString ExtraCgiParams;

        TImpl(const TString& domainPrefix, const TString& extraCgiParams, const TString& scheme, ui16 port)
            : Hosts()
            , Scheme(scheme.size() ? scheme : "https://")
            , SearchDomainPrefix(domainPrefix.size() ? domainPrefix : "hamster.yandex.")
            , Port(port ? port : Scheme == "https://" ? 443 : 80)
            , ExtraCgiParams(extraCgiParams)
        {
        }
    };

    TDomHosts::TDomHosts(const TString& searchDomainPrefix, const TString& extraCgiParams, const TString& scheme, ui16 port)
      : Impl(new TImpl(searchDomainPrefix, extraCgiParams, scheme, port))
    {
    }

    TDomHosts::~TDomHosts() {
    }

    const THost& TDomHosts::Get(const TString& domRegion) {
        TString dr = domRegion == "tr" ? "com.tr" : domRegion;
        TImpl::THosts::const_iterator i = Impl->Hosts.find(dr);
        if (i != Impl->Hosts.end()) {
            return i->second;
        }
        THost& res = Impl->Hosts[dr];
        res.Scheme = Impl->Scheme;
        res.Host = Impl->SearchDomainPrefix + dr;
        res.Port = Impl->Port;
        res.ExtraCgiParams = Impl->ExtraCgiParams;
        res.FillAddr();
        return res;
    }

    bool ParseUrl(TStringBuf url, TStringBuf& scheme, TStringBuf& hostport, TStringBuf& path, TStringBuf& cgi) {
        if (!url.size()) {
            return false;
        }
        if (url.StartsWith(TStringBuf("http://"))) {
            scheme = TStringBuf(url.data(), url.data() + 7);
            url = TStringBuf(url.data() + 7, url.data() + url.size());
        } else if (url.StartsWith(TStringBuf("http2://"))) {
            scheme = TStringBuf(url.data(), url.data() + 8);
            url = TStringBuf(url.data() + 8, url.data() + url.size());
        } else if (url.StartsWith(TStringBuf("https://"))) {
            scheme = TStringBuf(url.data(), url.data() + 8);
            url = TStringBuf(url.data() + 8, url.data() + url.size());
        } else {
            return false;
        }
        size_t p = url.find('/');
        if (p == TString::npos) {
            return false;
        }
        hostport = TStringBuf(url.data(), url.data() + p);
        url = TStringBuf(url.data() + p, url.data() + url.size());
        p = url.find('?');
        if (p == TString::npos) {
            return false;
        }
        path = TStringBuf(url.data(), url.data() + p + 1);
        cgi = TStringBuf(url.data() + p + 1, url.data() + url.size());
        return true;
    }

    bool ParseHostport(TStringBuf scheme, TStringBuf hostport, THost& host) {
        size_t p = hostport.find(':');
        if (p != TString::npos) {
            host.Host = TStringBuf(hostport.data(), hostport.data() + p);
            host.Port = FromString<ui16>(TStringBuf(hostport.data() + p + 1, hostport.data() + hostport.size()));
        } else {
            host.Host = hostport;
            host.Port = scheme == "https://" ? 443 : 80;
        }
        host.Scheme = scheme;
        host.FillAddr();
        return host.CanFetch();
    }

    bool ParseUrl(TStringBuf url, THost& host, TStringBuf& scheme, TStringBuf& hostport, TStringBuf& path, TStringBuf& cgi) {
        if (!ParseUrl(url, scheme, hostport, path, cgi)) {
            return false;
        }
        return ParseHostport(scheme, hostport, host);
    }

    bool ParseUrl(TStringBuf url, THost& host, TStringBuf& path, TStringBuf& cgi) {
        TStringBuf scheme;
        TStringBuf hostport;
        return ParseUrl(url, host, scheme, hostport, path, cgi);
    }

    bool BecomeHamster(THost& host) {
        TString shost = host.Host;
        TStringBuf hamster = "hamster.";
        size_t t = shost.find("yandex.");
        if (t == TString::npos || t > 0 && shost[t - 1] != '.') {
            return false;
        }
        if (t >= hamster.size() && TStringBuf(shost.data() + t - hamster.size(), shost.data() + t) == hamster) {
            if (t == hamster.size() || shost[shost[t - hamster.size() - 1] == '.']) {
                return true;
            }
        }
        TString res;
        res.append(shost.data(), shost.data() + t);
        res.append(hamster.data(), hamster.data() + hamster.size());
        res.append(shost.data() + t, shost.data() + shost.size());
        host.Host = res;
        host.RefillAddr();
        return host.CanFetch();
    }

}
