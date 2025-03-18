#include "stdafx.h"
#include "http_fetch.h"
#include <library/cpp/http/io/stream.h>
#include <util/network/socket.h>
#include <library/cpp/uri/http_url.h>
#include <library/cpp/charset/recyr.hh>

TString ToKoi8(const TString &szQuery) {
#ifdef _win32_
    wchar_t wszQuery[1024];
    int n = MultiByteToWideChar(1251, 0, szQuery.c_str(), szQuery.length(), wszQuery, 1000);
    wszQuery[n] = 0;
    char szQueryKoi[1024];
    n = WideCharToMultiByte(20866, 0, wszQuery, n, szQueryKoi, 1000, 0, 0);
    szQueryKoi[n] = 0;
    return TString(szQueryKoi);
#else
    return Recode(CODES_WIN, CODES_KOI8, szQuery);
#endif
}


TString ToUtf(const TString &szQuery) {
    return Recode(CODES_WIN, CODES_UTF8, szQuery);
}

TString FetchUrl(const char *szUrl, bool use_xml ) {
    TString szRes;
    try {
        THttpURL pUrl;
        if (pUrl.Parse(szUrl) != THttpURL::ParsedOK || pUrl.IsNull(THttpURL::FlagHost))
            return "";
        TString relUrl = pUrl.PrintS(THttpURL::FlagPath | THttpURL::FlagQuery);
        TNetworkAddress addr(pUrl.Get(THttpURL::FieldHost), pUrl.GetPort());
        TSocket s(addr);

        SendMinimalHttpRequest(s, pUrl.Get(THttpURL::FieldHost), relUrl.data(), "test_relev");

        TSocketInput si(s);
        THttpInput hi(&si);
        unsigned nRet = ParseHttpRetCode(hi.FirstLine());
        if (nRet != 200) {
            printf("http (use_xml = %d) request %s failed with return code %d\n", (int)use_xml, szUrl, nRet);
            return "";
        }
        TStringOutput so(szRes);
        TransferData(&hi, &so);
    } catch (...) {
        printf("Exception: %s\n", CurrentExceptionMessage().data());
    }
    return szRes;
}

