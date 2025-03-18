#include "stdafx.h"

#include <library/cpp/string_utils/old_url_normalize/url.h>
#include <library/cpp/html/pcdata/pcdata.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <util/system/thread.h>
#include <library/cpp/deprecated/atomic/atomic.h>

#include "SearchDuplicates.h"
#include "FetchYaResults.h"
#include "http_fetch.h"

const int N_THREADS = 10;

static TString ExtractTitle(const TString &szHtml) {
    string sz = szHtml.c_str();
    for (size_t i = 0; i < sz.length(); ++i)
        sz[i] = tolower(sz[i]);
    size_t nTitleStart = sz.find("<title>"), nTitleFinish = sz.find("</title>");
    if (nTitleStart == string::npos || nTitleFinish == string::npos)
        return "";
    nTitleStart += 7;
    return szHtml.substr(nTitleStart, nTitleFinish - nTitleStart);
}

static TString GetRHost(const char *pszUrl) {
    TString szUrl = pszUrl;
    if (szUrl.substr(0, 7) == "http://")
        szUrl = szUrl.substr(7);
    if (szUrl.substr(0, 8) == "https://")
        szUrl = szUrl.substr(8);
    size_t n = szUrl.find('/');
    if (n != TString::npos)
        szUrl = szUrl.substr(0, n);
    TVector<TString> parts;
    for(;;) {
        size_t n = szUrl.find('.');
        if (n == TString::npos)
            break;
        parts.push_back(szUrl.substr(0, n));
        szUrl = szUrl.substr(n + 1);
    }
    TString szRes = szUrl;
    for (int i = (int)parts.size() - 1; i >= 0; --i)
        szRes = szRes + "." + parts[i];
    return szRes;
}

static void FindDup(const char *_pszUrl, TString *pszRes) {
    *pszRes = "";

    TString szHtml = FetchUrl(_pszUrl);
    if (szHtml.empty())
        return;

    TString szTitle = ExtractTitle(szHtml);
    TString szHost = GetRHost(_pszUrl);
    const char *pszYaUrlFormat = "http://%s/xmlsearch?xml=da&text=%s&g=1.d.10.1.-1";
    TString szQuery = ToKoi8(szTitle) + "<< (rhost=\"" + szHost + "\"|rhost=\"" + szHost + ".*\")";
    CGIEscape(szQuery);
    char szYaUrl[2048];
    snprintf(szYaUrl, sizeof(szYaUrl) - 1, pszYaUrlFormat, LARGE_YA_XML_SEARCH_HOST, szQuery.c_str());

    // search with yandex for this url
    TString szRes = FetchUrl(szYaUrl);

    //printf("%s", szRes.c_str());
    printf("Fetched: %s\n", _pszUrl);

    size_t nPos = 0;
    for (int nTake = 0; nTake < 4; ++nTake) {
        nPos = szRes.find("<url>", nPos + 1);
        if (nPos == TString::npos)
            break;
        int nFinPos = szRes.find("</url>", nPos);
        TString szFoundDoc = szRes.substr(nPos + 5, nFinPos - nPos - 5);
        szFoundDoc = DecodeHtmlPcdata(szFoundDoc);

        TString szTestHtml = FetchUrl(szFoundDoc.c_str());
        if (szTestHtml == szHtml) {
            const int N_LARGE_BUF = 10000;
            char szNormalizedUrl[N_LARGE_BUF];
            if (NormalizeUrl(szNormalizedUrl, N_LARGE_BUF, szFoundDoc.c_str())) {
                *pszRes = szNormalizedUrl;
                return;
            }
        }
    }
}

static TAtomic nTaskId;
static const TVector<TString> *pSrc;
static void* WorkerThread(void *p) {
    TVector<TString> *pRes = (TVector<TString>*)p;
    for (;;) {
        long nTask = AtomicAdd(nTaskId, 1) - 1;
        if ((unsigned)nTask >= pRes->size())
            break;
        FindDup((*pSrc)[nTask].c_str(), &(*pRes)[nTask]);
    }
    return nullptr;
}

void FindDuplicates(const TVector<TString> &urls, TVector<TString> *pRes) {
    pSrc = &urls;
    nTaskId = 0;

    pRes->resize(urls.size());
    TVector<TThread*> FetchThreads;
    for (int i = 0; i < N_THREADS; i++) {
        FetchThreads.push_back(new TThread(WorkerThread, pRes));
        FetchThreads[i]->Start();
        usleep(100);
    }
    for (unsigned i = 0; i < FetchThreads.size(); i++)
        delete FetchThreads[i];

    pSrc = nullptr;
}
