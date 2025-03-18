#pragma once

#include "xmlsearchin.h"

#include <util/generic/string.h>
#include <util/generic/ptr.h>

namespace NSnippets {
    class THost {
    public:
        TString Scheme;
        TString Host;
        ui16 Port;
        TString ExtraCgiParams;

    private:
        struct TFetchImpl;
        THolder<TFetchImpl> FetchImpl;

    public:
        THost();
        THost(const THost& other);
        THost& operator=(const THost& other);
        ~THost();

        void FillAddr();
        void RefillAddr();
        bool CanFetch() const;

        TString Fetch(const TString& requestCgi, int follow = 0) const;
        bool XmlSearch(NXmlSearchIn::TRequest &res, const TString& text, const TString& source, const TString& xmltype, const TString& lr, const TString& groupby = TString()) const;
        TString RawSearch(const TString& endpoint, const TString& text, const TString& lr, const TString& uil, const TString& groupby = TString()) const;
    };

    class TDomHosts {
    private:
        struct TImpl;
        THolder<TImpl> Impl;

    public:
        TDomHosts(const TString& xmlPrefix = TString(), const TString& extraCgiParams = TString(), const TString& scheme = TString(), ui16 port = 0);
        ~TDomHosts();

        const THost& Get(const TString& domRegion);
    };

    bool ParseHostport(TStringBuf scheme, TStringBuf hostport, THost& host);
    bool ParseUrl(TStringBuf url, TStringBuf& scheme, TStringBuf& hostport, TStringBuf& path, TStringBuf& cgi);
    bool ParseUrl(TStringBuf url, THost& host, TStringBuf& scheme, TStringBuf& hostport, TStringBuf& path, TStringBuf& cgi);
    bool ParseUrl(TStringBuf url, THost& host, TStringBuf& path, TStringBuf& cgi);
    bool BecomeHamster(THost& host);
}
