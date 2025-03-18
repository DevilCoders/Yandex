#pragma once

#include <library/cpp/langs/langs.h>
#include <library/cpp/http/fetch/httpload.h>
#include <library/cpp/html/pcdata/pcdata.h>

#include <util/generic/string.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/printf.h>

TString GetDocumentByUrl(const TString &url, ExtHttpCodes &httpCode, time_t toutInSecs, bool needHttpCode)
{
    if (url.empty()) {
        httpCode = HTTP_BAD_URL;
        return ("");
    }
    httpLoadAgent agent;
    agent.SetTimeout(TDuration::Seconds(toutInSecs));
    agent.SetIdentification("User-Agent: Mozilla/5.0 Gecko/2009032711 Firefox/3.0.8", nullptr);
    agent.addHeaderInstruction("Accept: */*\r\n");
    agent.startRequest(url.c_str(), "");

    // Errors like authorisation request and similar stuff
    if (agent.error()) {
        httpCode = HTTP_BAD_URL;
        return ("");
    }

    // Errors like 404, 302, etc
    httpCode = (ExtHttpCodes) agent.getHeader()->http_status;
    if (httpCode != (ExtHttpCodes)HTTP_OK)
        return ("");

    TString result = "";
    while (!agent.eof()) {
        void * buff;
        long read;
        read = agent.readPortion(buff);

        if (!buff || read <= 0)
            break;

        result.append((char*)buff, read);
    }
    if (needHttpCode)
        httpCode = (ExtHttpCodes) agent.getHeader()->http_status;
    else
        httpCode = (ExtHttpCodes)HTTP_OK;
    return (result);
}

TString GetFullQuery(const TString &text)
{
    TString value = "";
    TString start = "<full-query>";
    TString end = "</full-query>";
    bool inTag = false;
    for(size_t i = 0; i < text.size() - start.size() - 1; ++i) {
        if (text.substr(i, start.size()) == start) {
            i += start.size() - 1;
            inTag = true;
            continue;
        }
        if (text.substr(i, end.size()) == end)
            return (value);
        if (inTag)
            value.push_back(text[i]);
    }
    return ("");
}

TString GetQtree(TString query, ELanguage lang)
{
    CGIEscape(query);
    TString cgiQuery = "http://xmlsearch.hamster.yandex.ru/xmlsearch?text=%s&full-query=1&srcask=WEB&srcrwr=WEB:WEB_FAKE";
    if (lang == LANG_TUR) {
        cgiQuery = cgiQuery + "&lr=983&l10n=tr";
    } else if (lang == LANG_UKR) {
        cgiQuery = cgiQuery + "&lr=187";
    } else {    // by default use russian wizard
        cgiQuery = cgiQuery + "&lr=213";
    }
    TString url = Sprintf(cgiQuery.data(), query.data());

    ExtHttpCodes httpCode;
    TString xmlText = GetDocumentByUrl(url, httpCode, 5, false);

    TString fullQuery = GetFullQuery(xmlText);
    TCgiParameters cgiParam(DecodeHtmlPcdata(fullQuery).data());
    return (cgiParam.Get("qtree"));
}

