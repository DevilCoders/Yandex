#include "stdafx.h"

#include <ctime>

#include <library/cpp/string_utils/old_url_normalize/url.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/charset/recyr.hh>
#include <util/string/printf.h>
#include <library/cpp/html/pcdata/pcdata.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/thread.h>

#include "FetchYaResults.h"
#include "ReadAssessData.h"
#include "http_fetch.h"

#define N_QUERY_THREADS_COUNT 10

const char *szMetaSearchAddress;

bool bTakeSingleWordQuery = true;
bool bTakeMultipleWordQuery = true;
bool bUseSlowMode = false;
TString additionalCgiParams;

static TVector<SQueryAnswer> *pQueryResults;
static TAtomic nFailedRequests;
static TAtomic nQueueId;
static EFetchType fetchType;
static TString szDebugOutputDir;

static const char *spamHosts[] = {
"www.iisikt.ru",
"kurort.art-market.ru",
"allsoch.com",
"e-sale.ru",
"vsenasvete.com",
"www.litera.info",
"www.razvlechenia.biz",
"www.informatsiya.info",
"www.promsites.biz",
"gosudarstvo.org",
"www.informatsija.info",
"rubook.info",
"www.rutabs.com",
"www.zvonit.ru",
"mp3-free.info",
"www.nii-katalog.ru",
"www.nauka.info",
"www.rusotyristo.ru",
"www.melodysearch.ru",
"mp3nox.info",
"portal-uslug.com",
"www.formatmp3.net",
"tipograf.smssms.ru",
"www.erudition.ru",
"media.gsmtop.ru",
"razvlechenia.com",
"www.razvlechenia.com",
"www.book.sgg.ru",
"bobych.ru",
"shop.kleo.ru",
"www.biz-list.ru",
"www.2devochki.ru",
"www.informatsija.info",
"www.megamost.ru",
"media.jp-net.ru",
"www.refstar.ru",
"www.nauka.ws",
"oracle.lib.unn.runnet.ru",
"anu.snud.ru",
"ruscore.ru",
"www.bookland.ru",
"nskpages.info",
"bookashop.ru",
"www.magazinov.net",
"ref.ewreka.ru",
"gde.ru",
"www.jga.ru",
"katalog.107-8.ru",
"www.medkatalog.info",
"text.subs.ru",
"forumford.nm.ru",
"logs.wallst.ru",
"intepnet.biz",
"www.allwebsites.ru",
"52.metakey.ru",
"www.wdir.ru",
"www.goo-liz.info",
"hop12.gaz-gaz.ru",

"poezdkafrance.front.ru",
"catalog.ihere.ru",
"www.bosyak.ru",
"permanent-poll.zhitomir.ua",
"toqj.cnrae.nii-ros.ru",
"turkey.travel-forum.ru",
"www.newchrono.ru",
"www.pro-links.net",
"www.x-links.ws",
"alko.net.ru",
"rus-biz.ru",
"www.12345678910.ru",
"www.autoshin.ru",
"www.intercompany.ru",
"www.chelprom.ru",
"mlm-shiv.nm.ru",
"ukrboard.com.ua",
"www.grafft.ru",
"digital.RuBid.net",
"togg.ydmq.nii-ros.ru",
"dom-metalla.37.ru",
"narodnyi.yzx.be",
"lookup.ru",
"technoco.ru",
"www.cybertown.ru",
"www.findout.ru",
"uri.alistis.org",
"www.onlineshopping.ru",
"ne-dvizhimost.bigcatalogonline.info",
"razin.net",
"ref.ewreka.ru",
"bankreferatoff.ru",
"www.qaz.ru",
"rzd.xost.ru",
"savat.lo-la.info",
"tofcv.hwiam.nii-ros.ru",
"loveinrussia.be",
"poxe.ru",
"razvod.makings.kiev.ua",
"sevntu.be",
"softsearch.ru",
"adsar.nm.ru",
"denova.nm.ru",
"purgen2005.nm.ru",

"info.funa.ru",
"www.promsites.biz"

// katalogs
"www.vsenaydem.ru",
"addsait.com",
"web-directory.ru",
"urloff.net",
"pornoshok.ru",
"odika.net",
"www.filmovnik.ru",
"kil.laxurl.org",
"service.polym.ru",
"level.polym.ru",
"web.4use.ru",
"nazapad.info",
"na-otdyhe.ru",
"www.frezer.info",
"www.stanochnik.info",
"kirpichiki.info",
"www.iula.ru",
"prodaem.ru",
"sportbox.info",
"tocyprus.spb.ru",
"vystavka.info",

// 20 jul
"i-catalog.com.ua",
"www.linktech.ru",
"www.center-life.ru",
"www.rucity.ru",
"marker.com.ua",
"elisting.com.ua",
"guide.com.ua",
"weblist.com.ua",
"datasearch.com.ua",
"adclick.com.ua",
"catalog.myweb.ru",
"www.linknews.ru",
"alllinks.com.ua",
"telerabota.com.ua",
"mamam.info",
"www.rollix.ru",
"naperstok.info",
"www.damam.info",
"www.ukrashenia.com",
"www.e-torg.info",
nullptr
};
static void FilterSpam(SQueryAnswer *pRes, int nMaxHosts) {
    int nDst = 0;
    for (size_t i = 0; i < pRes->results.size(); ++i) {
        const SUrlEstimates &ue = pRes->results[i];
        bool bSpam = false;
        for (const char **pszHost = spamHosts; *pszHost; ++pszHost) {
            if (strstr(ue.szUrl.c_str(), *pszHost)) {
                bSpam = true;
                break;
            }
        }
        if (bSpam)
            continue;
        pRes->results[nDst++] = ue;
    }
    if (nDst > nMaxHosts)
        nDst = nMaxHosts;
    pRes->results.resize(nDst);
}

static ui32 ExtractDebugComponent(const TString &szRes, size_t *pnRC) {
    size_t &nRC = *pnRC;
    nRC = szRes.find("<_RelevComponents>", nRC);
    ui32 nRes = 0;
    if (nRC != TString::npos) {
        int nFinPos = szRes.find("</_RelevComponents>", nRC);
        TString szNumber = szRes.substr(nRC + 18, nFinPos - nRC - 18);
        nRes = (ui32)atof(szNumber.c_str());
        nRC = nFinPos;
    }
    return nRes;
}


static void GetDomain(TString &szUrl,TString *szDomain) {
    size_t nPos = 0;
    if (strncmp(szUrl.data(), "http://", 7) == 0)
        nPos = 7;
    size_t nPos2 = szUrl.find('/', nPos);
    if (nPos2 == TString::npos)
        szDomain->assign(szUrl.c_str() , nPos, TString::npos);
    else
        szDomain->assign(szUrl.c_str() , nPos, nPos2 - nPos);
}

static bool GetUrl(TString &szRes,size_t *lastPos,TString *szUrl) {
/*
url
<div class="title">
<a tabindex="10" onclick="w(this, '80.22.82', '84=93');" href="http://fas.gov.ru/article/a_10036.shtml" target="_blank">ФАС России | Panasonic пострадал за <b>Media</b> <b>Markt</b></a>
</div>
*/
    size_t nPos = *lastPos;
    // looking for first <a href
    size_t nPos_a = szRes.find("<a tabindex=", nPos);
    if (nPos_a == TString::npos)
        return false;
    //looking for /a>
    size_t nPos_a_end = szRes.find("</a>", nPos_a);
    if (nPos_a_end == TString::npos)
        return false;
    // a теперь ищем href который внутри <a  /a>
    size_t nPos_href_url = szRes.find("href=\"", nPos_a);
    if ((nPos_href_url == TString::npos) || (nPos_href_url >= nPos_a_end) )  // где то за тегом <a /a> -
        return false;

    nPos_href_url += strlen("href=\"");  //

    size_t nPos_href_end = szRes.find("\" ", nPos_href_url);
    if (nPos_href_end == TString::npos)
        return false;

    szUrl->assign(szRes.c_str(), nPos_href_url, nPos_href_end - nPos_href_url);
    *lastPos = nPos_a_end;

    return true;
};


static bool GetUrlGoogle(TString &szRes,size_t *lastPos,TString *szUrl) {
/*
url
<h2 class=r><a href="http://www.danerd.com/media/2689_Hahn+Super+dry+sexy+super+honest+refresh" class=l>
*/
    size_t nPos = *lastPos;
    // looking for first <a href
    size_t nPos_a = szRes.find("<h2 class=r><a", nPos);
    if (nPos_a == TString::npos) {
        nPos_a = szRes.find("<div class=g><a", nPos);
        if (nPos_a == TString::npos)
            return false;
    };
    //looking for /a>
    // a теперь ищем href который внутри <a  class=l>>
    size_t nPos_href_url = szRes.find("href=\"", nPos_a);
    if ((nPos_href_url == TString::npos))  // где то за тегом <a /a> -
        return false;

    nPos_href_url += strlen("href=\"");  //

    size_t nPos_href_end = szRes.find("\" ", nPos_href_url);
    if (nPos_href_end == TString::npos)
        return false;

    szUrl->assign(szRes.c_str(), nPos_href_url, nPos_href_end - nPos_href_url);
    *lastPos = nPos_href_end;

    return true;
};


static bool GetUrlMsn(TString &szRes,size_t *lastPos,TString *szUrl) {
/*
url
<h3><a href="http://www.vodkavolk.com/" gping=
*/
    size_t nPos = *lastPos;
    // looking for first <a href
    size_t nPos_a = szRes.find("<h3><a ", nPos);
    if (nPos_a == TString::npos)
        return false;
    //looking for /a>
    // a теперь ищем href который внутри <a  class=l>>
    size_t nPos_href_url = szRes.find("href=\"", nPos_a);
    if ((nPos_href_url == TString::npos))  // где то за тегом <a /a> -
        return false;

    nPos_href_url += strlen("href=\"");  //

    size_t nPos_href_end = szRes.find("\" ", nPos_href_url);
    if (nPos_href_end == TString::npos)
        return false;

    szUrl->assign(szRes.c_str(), nPos_href_url, nPos_href_end - nPos_href_url);
    *lastPos = nPos_href_end;

    return true;
};


static bool GetUrlYahoo(TString &szRes,size_t *lastPos,TString *szUrl) {
/*
url
<a class=yschttl  href="http://rds.yahoo.com/_ylt=A0oGkkNdnShGBVAA2Q9XNyoA;_ylu=X3oDMTE5ODRrcmJ1BGNvbG8DdwRsA1dTMQRwb3MDMgRzZWMDc3IEdnRpZANNQVAwMDlfMTAx/SIG=11b4h5chu/EXP=1177153245**http%3a//vodka.report.ru/"><b>Водка</b> | Новости</a>
*/
    size_t nPos = *lastPos;
    // looking for first <a href
    //size_t nPos_a = szRes.find("<a class=yschttl  href=\"http://rds.yahoo.com/_", nPos);
    size_t nPos_a = szRes.find("<a class=yschttl ", nPos);
    if (nPos_a == TString::npos)
        return false;
    //looking for /a>
    // a теперь ищем href который внутри <a  class=l>>
    size_t nPos_href_url = szRes.find("href=\"", nPos_a);
    if ((nPos_href_url == TString::npos))  // где то за тегом <a /a> -
        return false;

    nPos_href_url += strlen("href=\"");  //

    size_t nPos_href_end = szRes.find("\"", nPos_href_url);
    if (nPos_href_end == TString::npos)
        return false;

    szUrl->assign(szRes.c_str(), nPos_href_url, nPos_href_end - nPos_href_url);
    *lastPos = nPos_href_end;

    return true;
}

static bool GetUrlGogo(TString &szRes, size_t *lastPos, TString *szUrl)
{
    /*
    url
    <div class="oTitle"><a target="_blank" href="blah">
    */
    // looking for first <a href
    const TString urlPrefix = "<div class=\"oTitle\"><a target=\"_blank\" href=\"";
    size_t posStart = szRes.find(urlPrefix, *lastPos);
    if (posStart == TString::npos)
        return false;

    posStart += urlPrefix.size();
    size_t posEnd = szRes.find("\">", posStart);
    if (posEnd == TString::npos)
        return false;

    *szUrl = szRes.substr(posStart, posEnd - posStart);
    *lastPos = posEnd;

    return true;
}

void GetWizardedRequest(TString &szRes, TString &szOut) {
// <br>Переколдованный запрос: <b>ыуч::1819103916</b>
    size_t nPos_1 = szRes.find("<br>��������������� ������: <b>");
    if (nPos_1 == TString::npos)
        return;
    nPos_1 += strlen("<br>��������������� ������: <b>");
    size_t nPos_end = szRes.find("</b>", nPos_1);
    if (nPos_end == TString::npos)
        return;
    TString tmp;
    tmp.assign(szRes.c_str(), nPos_1, nPos_end - nPos_1);
    // replace &amp; , &quot
    szOut = DecodeHtmlPcdata(tmp);
};


static void ParseHtml(TString &szRes, SQueryAnswer *pRes) {
try {
    pRes->szWizardedQuery = "";
    pRes->szCgiParams = "";
    size_t nPos = 0;
    ui32 nRank = 0, nTR = 0, nLR = 0, nPrior1 = 0, nPrior2 = 0, nHitWeight = 0, nMaxFreq = 0, nSumIdf = 0;
    ui32 nTRFlags = 0, nErfFlags = 0, nThemeMatch = 0;
    TString szGroup = "??";
    TString szUrl;
    const int N_LARGE_BUF = 10000;
    char szNormalizedUrl[N_LARGE_BUF];
    bool do_ret = false;
    if (fetchType == FT_HTML)  {
         TString szOut;
         GetWizardedRequest(szRes, szOut);
         if (szOut.size () != 0)
             (*pRes).szWizardedQuery = szOut;
    };

    while (true) {
        switch (fetchType) {
            case FT_HTML:
                if (!GetUrl(szRes, &nPos, &szUrl))
                    do_ret = true;
                break;
            case FT_HTML_GOOGLE:
                if (!GetUrlGoogle(szRes, &nPos, &szUrl))
                    do_ret = true;
                break;
            case FT_HTML_MSN:
                if (!GetUrlMsn(szRes, &nPos, &szUrl))
                    do_ret = true;
                break;
            case FT_HTML_YAHOO:
                if (!GetUrlYahoo(szRes, &nPos, &szUrl))
                    do_ret = true;
                break;
            case FT_HTML_GOGO:
                if (!GetUrlGogo(szRes, &nPos, &szUrl))
                    do_ret = true;
                break;
            default:
                ythrow yexception() << "Wrong fetch type";
        }
        if (do_ret)
            break;

        while (!szUrl.empty() && szUrl.back() == ' ')
            szUrl.resize(szUrl.size() - 1);
        GetDomain(szUrl, &szGroup);
        if (!NormalizeUrl(szNormalizedUrl, N_LARGE_BUF, szUrl.c_str()))
            continue;
        pRes->results.push_back(SUrlEstimates(szNormalizedUrl, szGroup.c_str(), nRank, nTR, nLR, nPrior1, nPrior2, nHitWeight, nMaxFreq, nSumIdf, nTRFlags, nErfFlags, nThemeMatch));
    }
} catch (...) {
    printf("Fatal error : Unknown exception catched ");
}
}


static void ParseXml(TString &szRes, bool bDbgRlv, int nOutputUrlsCount, SQueryAnswer *pRes) {
    size_t nPatchedQuery = szRes.find("<query>");
    if (nPatchedQuery != TString::npos) {
        size_t nFin = szRes.find("</query>", nPatchedQuery);
        if (nFin != TString::npos) {
            size_t nLength = nFin - nPatchedQuery - 7;
            pRes->szWizardedQuery = szRes.substr(nPatchedQuery + 7, nLength);
            pRes->szWizardedQuery = DecodeHtmlPcdata(pRes->szWizardedQuery);
            pRes->szWizardedQuery = Recode(CODES_UTF8, CODES_WIN, pRes->szWizardedQuery);
        }
    }
    size_t nCgiParams = szRes.find("<full-query>");
    if (nCgiParams != TString::npos) {
        size_t nFin = szRes.find("</full-query>", nCgiParams);
        if (nFin != TString::npos) {
            size_t nLength = nFin - nCgiParams - 12;
            pRes->szCgiParams = szRes.substr(nCgiParams + 12, nLength);
            pRes->szCgiParams = DecodeHtmlPcdata(pRes->szCgiParams);
            pRes->szCgiParams = Recode(CODES_UTF8, CODES_WIN, pRes->szCgiParams);
        }
    }
    size_t nPos = 0;
    TString szGroup = "??";
    for (;;) {
        const char *pszGroupStart = "<categ attr=\"d\" name=\"";
        size_t nPosG = szRes.find(pszGroupStart, nPos);
        nPos = szRes.find("<url>", nPos + 1);
        if (nPos == TString::npos)
            break;
        if (nPosG != TString::npos && nPosG < nPos) {
            int nGroupPos = nPosG + strlen(pszGroupStart);
            int n1 = szRes.find("\"", nGroupPos);
            szGroup = szRes.substr(nGroupPos, n1 - nGroupPos);
        }
        int nFinPos = szRes.find("</url>", nPos);
        TString szFoundDoc = szRes.substr(nPos + 5, nFinPos - nPos - 5);
        szFoundDoc = DecodeHtmlPcdata(szFoundDoc);
        ui32 nRank = 0, nTR = 0, nLR = 0, nPrior1 = 0, nPrior2 = 0, nHitWeight = 0, nMaxFreq = 0, nSumIdf = 0;
        ui32 nTRFlags = 0, nErfFlags = 0, nThemeMatch = 0;
        if (bDbgRlv) {
            size_t nRC = nPos;
            nRank = ExtractDebugComponent(szRes, &nRC);
            nTR = ExtractDebugComponent(szRes, &nRC);
            nLR = ExtractDebugComponent(szRes, &nRC);
            nPrior1 = ExtractDebugComponent(szRes, &nRC);
            nPrior2 = ExtractDebugComponent(szRes, &nRC);
            nHitWeight = ExtractDebugComponent(szRes, &nRC);
            nMaxFreq = ExtractDebugComponent(szRes, &nRC);
            nSumIdf = ExtractDebugComponent(szRes, &nRC);
            nTRFlags = ExtractDebugComponent(szRes, &nRC);
            nErfFlags = ExtractDebugComponent(szRes, &nRC);
            nThemeMatch = ExtractDebugComponent(szRes, &nRC);
        }
        const int N_LARGE_BUF = 10000;
        char szNormalizedUrl[N_LARGE_BUF];
        if (!NormalizeUrl(szNormalizedUrl, N_LARGE_BUF, szFoundDoc.c_str()))
            continue;
        pRes->results.push_back(SUrlEstimates(szNormalizedUrl, szGroup.c_str(), nRank, nTR, nLR, nPrior1, nPrior2, nHitWeight, nMaxFreq, nSumIdf, nTRFlags, nErfFlags, nThemeMatch));
    }
#ifdef FILTER_KNOWN_SPAM
    FilterSpam(pRes, nOutputUrlsCount);
#else
    if (pRes->results.size() > nOutputUrlsCount)
        pRes->results.resize(nOutputUrlsCount);
#endif
}

static int FetchQuery(SQueryAnswer *pRes) {
    bool use_xml = false;
    if (bUseSlowMode) { //
        const TString& queryText = pRes->query.Text;
        unsigned char factor = (unsigned char)(queryText[0] + 2 * queryText[1] + 7 * queryText[3]);
        factor = (factor + 1) % 10;
        long random_time_out = factor + (rand() % (N_QUERY_THREADS_COUNT+ factor + 1)); //
        //printf(" BEFORE : time_out : %u, factor : %u \n", random_time_out, factor);
        sleep(random_time_out);
    }

    //TString szHost = "gulin_xp.ld.yandex.ru:17013";
    TString szHost(szMetaSearchAddress);
    TString szQuery = pRes->query.Text;
    bool bTake = false;
    if (szQuery.find_first_of(" -") != TString::npos)
        bTake = bTakeMultipleWordQuery;
    else
        bTake = bTakeSingleWordQuery;
    if (!bTake) {
        AtomicIncrement(nFailedRequests);
        return 0;
    }

    const char *pszUrlFormat = nullptr;
    bool bDbgRlv = false;
    bool yandex = false;
    int nDownloadUrlsCount = N_DOWNLOAD_URLS_COUNT;
    int nOutputUrlsCount = N_OUTPUT_URLS_COUNT;
    switch(fetchType) {
        case FT_SIMPLE:
            pszUrlFormat = "http://%s/xmlsearch?text=%s&g=1.d.%d.1.-1&xml=da"; // ya
            use_xml = true;
            yandex = true;
            break;
        case FT_RELFORM_DEBUG:
            pszUrlFormat = "http://%s/xmlsearch?text=%s&g=1.d.%d.1.-1&xml=da&pron=relform&full-query=WEB&gta=_RelevComponents"; // ya, relform
            bDbgRlv = true;
            use_xml = true;
            yandex = true;
            break;
        case FT_ATR_DEBUG:
            pszUrlFormat = "http://%s/xmlsearch?text=%s&g=1.d.%d.1.-1&xml=da&atr=1&np=da&full-query=WEB&gta=_RelevComponents"; // atr
            bDbgRlv = true;
            use_xml = true;
            yandex = true;
            break;
        case FT_SELECTED:
            pszUrlFormat = "http://%s/xmlsearch?text=%s&g=1.d.300.20.-1&xml=da&full-query=WEB&gta=_RelevComponents"; // ya, selected urls
            nDownloadUrlsCount = 0; // not used in this case
            nOutputUrlsCount = 6000; // output everything downloaded
            bDbgRlv = true;
            use_xml = true;
            yandex = true;
            break;
        case FT_ATR_SELECTED:
            pszUrlFormat = "http://%s/xmlsearch?text=%s&g=1.d.300.20.-1&xml=da&atr=1&np=1&full-query=WEB&gta=_RelevComponents"; // atr, selected urls
            nDownloadUrlsCount = 0; // not used in this case
            nOutputUrlsCount = 6000; // output everything downloaded
            bDbgRlv = true;
            use_xml = true;
            yandex = true;
            break;
        case FT_LARGE_YA:
            pszUrlFormat = "http://%s/xmlsearch?xml=da&text=%s&g=1.d.%d.1.-1";
            use_xml = true;
            yandex = true;
            break;

        case FT_HTML:
            pszUrlFormat = "http://%s/yandsearch?text=%s&dbgwzr=1"; // html
            yandex =  true;
            break;
        case FT_HTML_GOOGLE:  //http://www.google.com/search?q=
            pszUrlFormat = "http://%s/search?hl=ru&q=%s"; // html
            szQuery = ToUtf(szQuery);
            break;
        case FT_HTML_MSN:  //http://www.google.com/search?q=
            pszUrlFormat = "http://%s/results.aspx?q=%s"; // html
            szQuery = ToUtf(szQuery);
            break;
        case FT_HTML_YAHOO: // http://ru.search.yahoo.com/search?p=%D0%B2%D0%BE%D0%B4%D0%BA%D0%B0&ei=UTF-8
            pszUrlFormat = "http://%s/search?p=%s&ei=UTF-8"; // html
            szQuery = ToUtf(szQuery);
            break;
        case FT_HTML_GOGO:
            pszUrlFormat = "http://%s/go?q=%s"; // html
            break;

        default:
            assert(0);
            break;
    }

    CGIEscape(szQuery);

    char szUrlBuf[2048];
    if (fetchType == FT_HTML_GOOGLE ) { // there's some diffs between com and ru
        if (strcmp(szHost.c_str(),"www.google.com")==0)
            pszUrlFormat = "http://%s/search?hl=en&q=%s"; // com
        else
            pszUrlFormat = "http://%s/search?hl=ru&q=%s"; // ru
    }


    snprintf(szUrlBuf, sizeof(szUrlBuf) - 1, pszUrlFormat, szHost.c_str(), szQuery.data(), nDownloadUrlsCount);
    pRes->szFetchUrl = szUrlBuf + additionalCgiParams;

    if (yandex) {
        pRes->szFetchUrl += Sprintf("&lr=%d", static_cast<int>(pRes->query.Region));
    }

    TString szRes;
    for (int nRetry = 0; nRetry < 3; ++nRetry) {
        szRes = FetchUrl(pRes->szFetchUrl.c_str(), use_xml);
        if (!szRes.empty())
            break;
    }
    if (szRes.empty()) {
        AtomicIncrement(nFailedRequests);
        return 0;
    }
    //printf("%s", szRes.c_str());
    printf("Fetched: %s\n", szQuery.data());

    switch (fetchType) {
        case FT_HTML:
        case FT_HTML_GOOGLE:
        case FT_HTML_MSN:
        case FT_HTML_YAHOO:
        case FT_HTML_GOGO:
            ParseHtml(szRes, pRes);
            break;
        default:
            ParseXml(szRes, bDbgRlv, nOutputUrlsCount, pRes);
    }

    {
        char szFileName[1000];
        sprintf(szFileName, "%s/req%d", szDebugOutputDir.c_str(), pRes->nRequestId);

        TFixedBufferFileOutput f(szFileName);
        for (size_t i = 0; i < pRes->results.size(); ++i)
            f << pRes->results[i].szUrl.c_str() << "\n";
    }

    if (bUseSlowMode) { // wait 10 - 15 seconds for next request
        unsigned char factor = (unsigned char)(szQuery[0] + 4 * szQuery[3]);
        factor = (factor + 1) % 10;
        long random_time_out = 3 + factor + (rand() % 10); //
        sleep(random_time_out);
        //printf(" AFTER time_out : %u \n", random_time_out);
    };
    return 0;
}

static void* FetchQueries(void *p) {
    TVector<SQueryAnswer> *pRes = (TVector<SQueryAnswer> *)p;
    for (;;) {
        long nCurrentQuery = AtomicAdd(nQueueId, 1) - 1;
        if ((unsigned)nCurrentQuery >= pRes->size())
            break;
        SQueryAnswer *pDst = &(*pRes)[nCurrentQuery];
        FetchQuery(pDst);
    }
    return nullptr;
}

void FetchYandex(const TVector<TQuery> &queries, TVector<SQueryAnswer> *pRes, const char *pszDebugOutputDir, EFetchType _fetchType) {
    fetchType = _fetchType;
    szDebugOutputDir = pszDebugOutputDir;
    pQueryResults = pRes;
    nFailedRequests = 0;
    nQueueId = 0;

    int nMaxRequestsInFly = N_QUERY_THREADS_COUNT;
    if (fetchType == FT_SELECTED || fetchType == FT_ATR_SELECTED)
        nMaxRequestsInFly = 1;

    int nQueriesCount = queries.size();
    pRes->resize(nQueriesCount);
    for (int i = 0; i < nQueriesCount; ++i) {
        SQueryAnswer &a = (*pRes)[i];
        a.query = queries[i];
        a.nRequestId = i;
    }
    TVector<TThread*> FetchThreads;
    for (int i = 0; i < nMaxRequestsInFly; i++) {
        FetchThreads.push_back(new TThread(FetchQueries, pRes));
        FetchThreads[i]->Start();
        usleep(100);
    }
    for (unsigned i = 0; i < FetchThreads.size(); i++)
        delete FetchThreads[i];

    printf("%lu requests failed out of %lu\n", nFailedRequests, (unsigned long)queries.size());
}
