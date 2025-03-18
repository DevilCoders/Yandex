#include "stdafx.h"

#include "FetchYaResults.h"
#include "ReadAssessData.h"
#include "ReadResults.h"
#include "SearchDuplicates.h"
#include "UrlsToAssess.h"
#include "YaRelev.h"

#include <library/cpp/getopt/opt.h>

#include <util/network/socket.h>
#include <util/string/util.h>
#include <util/system/defaults.h>

#include <cmath>
#include <ctime>
#include <fstream>

struct SMissingUrl
{
    int nReqId, nDocId;
    double fWeight;
    SMissingUrl() : nReqId(-1), nDocId(-1), fWeight(0) {}
    SMissingUrl(int _nReqId, int _nDocId, double _fWeight) : nReqId(_nReqId), nDocId(_nDocId), fWeight(_fWeight) {}
};
//////////////////////////////////////////////////////////////////////////
static double CalcGMeasureInt(const TVector<SReqResult> &res, TVector<SMissingUrl> *pMissing)
{
    double fGMeasure = 0, fPlook = 1;
    THashMap<int,bool> seenGroups;
    int nPos = 0;
    for (int i = 0; i < (int)res.size(); ++i) {
        SReqResult r = res[i];
        if (seenGroups.find(r.GroupId) != seenGroups.end())
            continue;
        seenGroups[r.GroupId];

        ++nPos;

        if (r.MarkCount == 0) {
            if (pMissing)
                pMissing->push_back(SMissingUrl(-1, r.DocId, fPlook));
        }
        double fPfound = r.GetRelev();
        if (fPfound >= 0) {
            fGMeasure += fPlook * fPfound;
            fPlook = fPlook * (1 - fPfound) * 0.85;
        }
    }
    return fGMeasure;
}
static double CalcGMeasure(const TVector<SReqResult> &_estimated, const TVector<SReqResult> &_res, bool bUnknownAreRelev, TVector<SMissingUrl> *pMissing, double *pfGmax)
{
    TVector<SReqResult> est = _estimated;
    TVector<SReqResult> res = _res;
    if (bUnknownAreRelev) {
        assert(!pMissing);
        float fPprev = 1;
        for (size_t i = 0; i < res.size(); ++i) {
            SReqResult &r = res[i];
            if (r.MarkCount == 0) {
                r.SumRelev = fPprev;
                r.MarkCount = 1;
                est.push_back(r); // r.nRelevWeight == 0 means r.nDocId was not estimated
            } else {
                float fP = r.GetRelev();
                if (fP >= 0)
                    fPprev = fPprev * 0.8f + fP * 0.2f;
            }
        }
    }
    double fG = CalcGMeasureInt(res, pMissing);
    if (pfGmax) {
        std::sort(est.begin(), est.end(), SCmpRR());
        *pfGmax = CalcGMeasureInt(est, nullptr);
    }
    return fG;
}
//////////////////////////////////////////////////////////////////////////
static THashMap<int,THashMap<int,bool> > orRelev;
#ifdef _win32_
static void ReadOrRelev(const char *pszFileName)
{
    TFileInput fIn(pszFileName);
    TString line;
    while (fIn.ReadLine(line)) {
        if (line.empty())
            break;
        int nDocId, nReqId, nRel;
        sscanf(~line, "%d\t%d\t%d", &nDocId, &nReqId, &nRel);
        orRelev[nReqId][nDocId] = (nRel != 0);
    }
}
#endif
static double CalcAvrgPrecision(int nReqId, const TVector<SReqResult> &res) {
    double fAvrgPrecision = 0;
    int nFoundRel = 0, nTotalDocs = 0;
    const THashMap<int, bool> &relev = orRelev[nReqId];
    int nTotalRelev = 0;
    for (THashMap<int, bool>::const_iterator z = relev.begin(); z != relev.end(); ++z)
        nTotalRelev += z->second ? 1 : 0;
    for (int i = 0; i < (int)res.size(); ++i) {
        const SReqResult &r = res[i];
        THashMap<int,bool>::const_iterator iFind = relev.find(r.DocId);
        ++nTotalDocs;
        if (iFind == relev.end())
            continue;
        if (iFind->second) {
            if (nTotalDocs <= 50)
                fAvrgPrecision += (nFoundRel + 1.0) / nTotalDocs;
            ++nFoundRel;
        }
    }
    fAvrgPrecision /= nTotalRelev;
    if (nReqId == 10763)//1684)//
        fAvrgPrecision = fAvrgPrecision + 0;
    return fAvrgPrecision;
}
//////////////////////////////////////////////////////////////////////////
static void AnalyzeResults(const TResultHash &estimated, const TResultHash &reqResults,
    const TVector<float> &reqWeights, TVector<SMissingUrl> *pMissing,
    const char *pszFileName)
{
    double fGTotal = 0, fAvrgPrecision = 0, fGTotalOptimistic = 0, fTotalWeight = 0;
    double fGTotalBest = 0;
    std::ofstream *pfOut = pszFileName ? new std::ofstream(pszFileName) : nullptr;
    for (TResultHash::const_iterator i = reqResults.begin(); i != reqResults.end(); ++i) {
        int nReqId = i->first;
        TVector<SReqResult> res = i->second;
        TVector<SReqResult> estimatedRes;
        TResultHash::const_iterator iEst = estimated.find(nReqId);
        if (iEst != estimated.end())
            estimatedRes = iEst->second;

        double fGbest;
        double fG = CalcGMeasure(estimatedRes, res, false, pMissing, &fGbest);
        double fGopt = CalcGMeasure(estimatedRes, res, true, nullptr, nullptr);
        double fPrec = CalcAvrgPrecision(nReqId, res);
        if (pfOut)
            (*pfOut) << nReqId << "\t" << fG << "\t" << fGopt << "\t" << fPrec << std::endl;
        float fWeight = 1;
        if (!reqWeights.empty())
            fWeight = reqWeights[nReqId];
        fGTotal += fG * fWeight;
        fGTotalOptimistic += fGopt * fWeight;
        fGTotalBest += fGbest * fWeight;
        fAvrgPrecision += fPrec * fWeight;
        fTotalWeight += fWeight;

        if (pMissing) {
            for (int i = pMissing->size() - 1; i >= 0; --i) {
                if ((*pMissing)[i].nReqId == -1)
                    (*pMissing)[i].nReqId = nReqId;
                else
                    break;
            }
        }
    }
    fGTotal /= fTotalWeight;
    fGTotalOptimistic /= fTotalWeight;
    fGTotalBest /= fTotalWeight;
    fAvrgPrecision /= fTotalWeight;
    char szBuf[1024];
    sprintf(szBuf, "Pfound = %g%% - %g%%\nMax Pfound = %g%%\nAvrgPrecision = %g%%\n",
        fGTotal * 100, fGTotalOptimistic * 100, fGTotalBest * 100, fAvrgPrecision * 100);
    printf("%s", szBuf);
#ifdef _win32_
    OutputDebugString(szBuf);
#endif
}
//////////////////////////////////////////////////////////////////////////
#ifdef TEST_AVRG_PRECISION
static void TestAvrgPrecision()
{
    THashMap<int, TVector<int> > sentResults;
    {
        std::ifstream fIn("C:\\ya\\indexRomip\\yaSentResults.txt");
        while (!fIn.eof()) {
            char szBuf[1024];
            fIn.getline(szBuf, 1000);
            int nReqId, nDocId;
            sscanf(szBuf, "%d\t%d", &nReqId, &nDocId);
            sentResults[nReqId].push_back(nDocId);
        }
    }
    int nReqId = 5616, nFakeGroupId = 0;
    TVector<SReqResult> res;
    TVector<int> src = sentResults[nReqId];
    for (int i = 0; i < (int)src.size(); ++i)
        res.push_back(SReqResult(src[i], nFakeGroupId++, 0, 0));
    double fTestAP = CalcAvrgPrecision(nReqId, res);
    printf("Test AP = %g\n", fTestAP);
}
#endif
//////////////////////////////////////////////////////////////////////////
struct SWeightedDoc {
    int nDoc;
    double fWeight;
    SWeightedDoc() {}
    SWeightedDoc(int _nDoc, double _fWeight) : nDoc(_nDoc), fWeight(_fWeight) {}
};
struct SWeightedDocCmp {
    bool operator()(const SWeightedDoc &a, const SWeightedDoc &b) const { return a.fWeight > b.fWeight; }
};
#ifdef _win32_
static void Merge(TResultHash *pRes, const TResultHash &rq1, const TResultHash &rq2)
{
    for (TResultHash::const_iterator i = rq1.begin(); i != rq1.end(); ++i) {
        int nReqId = i->first;
        const TVector<SReqResult> &res1 = i->second;
        TResultHash::const_iterator i2 = rq2.find(nReqId);
        if (i2 == rq2.end()) {
            (*pRes)[nReqId] = res1;
            continue;
        }
        const TVector<SReqResult> &res2 = i2->second;
        THashMap<int,double> weights;
        for (int i = 0; i < (int)res1.size(); ++i)
            weights[res1[i].DocId] += exp(-i * 0.1);
        for (int i = 0; i < (int)res2.size(); ++i)
            weights[res2[i].DocId] += exp(-i * 0.1);
        TVector<SWeightedDoc> ww;
        for (THashMap<int,double>::const_iterator k = weights.begin(); k != weights.end(); ++k)
            ww.push_back(SWeightedDoc(k->first, k->second));
        std::sort(ww.begin(), ww.end(), SWeightedDocCmp());
        TVector<SReqResult> dst;
        int nFakeGroupId = 0;
        for (size_t i = 0; i < ww.size(); ++i)
            dst.push_back(SReqResult(ww[i].nDoc, nFakeGroupId++, 0, 0));
        (*pRes)[nReqId] = dst;
    }
}
#endif
//////////////////////////////////////////////////////////////////////////
static void GenerateIndexSelectUrls(const char *pszAssessData)
{
    // load assessors estimates
    TVector<int> qAges;
    TVector<TQuery> queries;
    TVector<TString> docs;
    THashMap<TString,int> docIds;
    TResultHash estimates;
    SHost2Group host2group;
    ReadAssessData(pszAssessData, &queries, &qAges, &docs, &docIds, &host2group, &estimates, true);
    {
        std::ofstream ff("index.selecturls");
        for (THashMap<TString,int>::const_iterator i = docIds.begin(); i != docIds.end(); ++i) {
            TString szUrl = i->first;
            size_t n = szUrl.find("://");
            if (n != TString::npos) {
                szUrl = szUrl.substr(n + 3);
                szUrl = to_lower(szUrl);
                ff << szUrl.c_str() << std::endl;
            }
        }
    }
    return;
}
//////////////////////////////////////////////////////////////////////////
//static void AnalyzeMissing(const char *pszAssessData) {
//    // load assessors estimates
//    TVector<int> qAges;
//    TVector<TString> queries, docs;
//    THashMap<TString,int> docIds;
//    TResultHash estimates;
//    SHost2Group host2group;
//    ReadAssessData(pszAssessData, &queries, &qAges, &docs, &docIds, &host2group, &estimates);
//
//    // load missing list
//    THashMap<TString,bool> notFound;
//    {
//        std::ifstream fMis("C:/_MissingYou/out.txt");
//        while (!fMis.eof() && !fMis.fail()) {
//            TString szUrl;
//            char szBuf[102400];
//            fMis.getline(szBuf, 100000);
//            if (szBuf[0] == 0)
//                break;
//            char *pszNum = szBuf;
//            for (char *p = szBuf; *p; ++p) {
//                if (*p == 0x0d) {
//                    *p = 0;
//                    pszNum = p + 2;
//                    break;
//                }
//            }
//            szUrl = szBuf;;
//            bool bPresent = atoi(pszNum) != 0;
//            notFound[szUrl] = bPresent;
//        }
//    }
//
//    // output result
//    {
//        std::ofstream dstFile("notfound_info.txt");
//        for (TResultHash::const_iterator i = estimates.begin(); i != estimates.end(); ++i) {
//            TString szRequest = queries[i->first];
//            int nSpaces = 0;
//            for (size_t z = 0; z < szRequest.length(); ++z)
//                nSpaces += szRequest[z] == ' ';
//
//            const TVector<SReqResult> &rvec = i->second;
//            int nRelevCount = 0, nFoundRelev = 0;
//            for (size_t k = 0; k < rvec.size(); ++k) {
//                nRelevCount += rvec[k].nRelev != 0;
//                if (notFound.find(docs[rvec[k].nDocId]) == notFound.end())
//                    nFoundRelev += rvec[k].nRelev != 0;
//            }
//
//            for (size_t k = 0; k < rvec.size(); ++k) {
//                const SReqResult &rr = rvec[k];
//                THashMap<TString,bool>::const_iterator fi = notFound.find(docs[rvec[k].nDocId]);
//                if (fi == notFound.end())
//                    continue;
//                dstFile << docs[rr.nDocId].c_str() << "\t" << szRequest.c_str() << "\t" << nSpaces << "\t" << rr.nRelev << "\t";
//                dstFile << (fi->second ? "in_base\t" : "missing\t");
//                dstFile << nRelevCount << "\t" << nFoundRelev << std::endl;
//            }
//        }
//    }
//}
//////////////////////////////////////////////////////////////////////////
struct SNotFoundInfo {
    TQuery query;
    TString szUrl;
    double fRelev;
};
struct SCmpNotFoundInfo {
    bool operator()(const SNotFoundInfo &a, const SNotFoundInfo &b) const { return a.szUrl < b.szUrl; }
};
//////////////////////////////////////////////////////////////////////////
static void TestYandexWithAssessData(const char *pszAssessData, const TString &szLogDir, EFetchType ft, bool bOutputDbgRlv, bool storeMarks)
{
    // load assessors estimates
    TVector<int> qAges;
    TVector<TQuery> queries;
    TVector<TString> docs;
    THashMap<TString,int> docIds;
    TResultHash estimates;
    SHost2Group host2group;
    ReadAssessData(pszAssessData, &queries, &qAges, &docs, &docIds, &host2group, &estimates, storeMarks);

    TVector<float> reqWeights;
#ifdef REQUESTS_DECAY_WITH_TIME
    {
        reqWeights.resize(qAges.size());
        for (size_t i = 0; i < qAges.size(); ++i)
            reqWeights[i] = exp(-qAges[i] / 1000.0f);
    }
#endif

    // fetch results from yandex
    TVector<TQuery> rq;
    TVector<SQueryAnswer> answers;
    {
        for (size_t i = 0; i < queries.size(); ++i) {
            const TQuery& query = queries[i];
            rq.push_back(query);
        }
    }
    FetchYandex(rq, &answers, szLogDir.c_str(), ft);
    //ReadResults("C:/Code/CalcEp10OnAssess/!yaLarge30/", rq, &answers, &host2group, N_OUTPUT_URLS_COUNT);

    if (bOutputDbgRlv) {
        TFixedBufferFileOutput fOut((szLogDir + "/dbgrlv.txt").c_str());
        TFixedBufferFileOutput fMissing((szLogDir + "/dbgrlvMis.txt").c_str());
        for (size_t nQueryId = 0; nQueryId < queries.size(); ++nQueryId) {
            const SQueryAnswer &a = answers[nQueryId];
            const TVector<SReqResult> &estArray = estimates[nQueryId];
            for (size_t i = 0; i < a.results.size(); ++i) {
                const SUrlEstimates &ue = a.results[i];
                int nDocId = -1;
                THashMap<TString,int>::const_iterator z = docIds.find(ue.szUrl);
                bool bFound = false;
                double fRelev = 0;
                if (z != docIds.end()) {
                    nDocId = z->second;
                    SReqResult est = FindEstimate(estArray, nDocId, -1);
                    if (est.MarkCount != 0) {
                        bFound = true;
                        fRelev = est.GetRelev();
                    }
                }
                TFixedBufferFileOutput* pOut = bFound ? &fOut : &fMissing;
                (*pOut) << nQueryId;// << hash<const char*>()(ue.szUrl.c_str());
                (*pOut) << "\t" << fRelev;
                (*pOut) << "\t" << ue.szUrl.c_str() << "\t" << ue.szGroup.c_str();
                for (int nParam = 0; nParam < SUrlEstimates::N_PARAMS; ++nParam)
                    (*pOut) << "\t" << ue.fParams[nParam];
                (*pOut) << "\n";
            }
        }
    }

    {
        TFixedBufferFileOutput f((szLogDir + "/reqUrl.txt").c_str());
        for (size_t i = 0; i < answers.size(); ++i)
            f << answers[i].szFetchUrl.c_str() << "\n";
    }

    {
        TFixedBufferFileOutput f((szLogDir + "/serp.txt").c_str());
        for (size_t reqId = 0; reqId < answers.size(); ++reqId) {
            const SQueryAnswer &serp = answers[reqId];
            const TString &req = serp.query.Text;
            for (size_t k = 0; k < serp.results.size(); ++k) {
                const SUrlEstimates &est = serp.results[k];
                f << est.szUrl.c_str() << "\txrefLR\t" << k + 1 << "\t" << req.c_str() << "\n";
            }
        }
    }

    if (ft == FT_SELECTED || ft == FT_ATR_SELECTED) {
        THashMap<TString,SNotFoundInfo> urls;
        for (TResultHash::const_iterator i = estimates.begin(); i != estimates.end(); ++i) {
            const TVector<SReqResult> &rvec = i->second;
            int nReqId = i->first;
            for (size_t k = 0; k < rvec.size(); ++k) {
                const SReqResult &rr = rvec[k];
                SNotFoundInfo nf;
                nf.query = queries[nReqId];
                nf.fRelev = rr.GetRelev();
                urls[docs[rr.DocId]] = nf;
            }
        }
        for (size_t i = 0; i < answers.size(); ++i) {
            const SQueryAnswer &qa = answers[i];
            for (size_t k = 0; k < qa.results.size(); ++k) {
                const SUrlEstimates &e = qa.results[k];
                THashMap<TString,SNotFoundInfo>::iterator zz = urls.find(e.szUrl);
                if (zz != urls.end())
                    urls.erase(zz);
            }
        }
        TVector<SNotFoundInfo> notFound;
        for (THashMap<TString,SNotFoundInfo>::const_iterator i = urls.begin(); i != urls.end(); ++i) {
            SNotFoundInfo nf = i->second;
            nf.szUrl = i->first;
            notFound.push_back(nf);
        }
        std::sort(notFound.begin(), notFound.end(), SCmpNotFoundInfo());

        TFixedBufferFileOutput dstFile(szLogDir + "/notfound.txt");

        for (size_t i = 0; i < notFound.size(); ++i) {
            const SNotFoundInfo &nf = notFound[i];
            const TString& text = nf.query.Text;

            int nSpaces = 0;
            for (size_t z = 0; z < text.length(); ++z) {
                if (text[z] == ' ') {
                    ++nSpaces;
                }
            }

            dstFile << nf.szUrl << "\t" << text << "\t" << nf.fRelev << "\t" << nSpaces << "\t" << nf.query.Region << "\n";
        }
    }

    THashMap<int, TString> missingUrls;
    // fill pReqResults
    TResultHash reqResults;
    for (size_t nQueryId = 0; nQueryId < queries.size(); ++nQueryId) {
        const SQueryAnswer &a = answers[nQueryId];
        TVector<SReqResult> &rqRes = reqResults[nQueryId];
        for (size_t i = 0; i < a.results.size(); ++i) {
            int nDocId = -1;
            THashMap<TString,int>::const_iterator z = docIds.find(a.results[i].szUrl);
            if (z != docIds.end())
                nDocId = z->second;
            else {
                nDocId = -(int)missingUrls.size() - 1;
                missingUrls[nDocId] = a.results[i].szUrl;
            }
            // GetGroupId(should be a.results[i].szGroup), but for this we need group for each estimated url in assbase
            rqRes.push_back(SReqResult(nDocId, host2group.GetGroupIdByUrl(a.results[i].szUrl), 0, 0));
        }
    }
    FillRelev(&reqResults, estimates);

    TVector<SMissingUrl> missing;
    AnalyzeResults(estimates, reqResults, reqWeights, &missing, (szLogDir + "/reqStats.txt").c_str());
    {
        TFixedBufferFileOutput f((szLogDir + "/reqText.txt").c_str());
        for (size_t i = 0; i < rq.size(); ++i)
            f << rq[i].Text << "\t" << rq[i].Region << "\n";
    }

    TVector<SUrlToAssess> toAssess;
    for (size_t i = 0; i < missing.size(); ++i) {
        const SMissingUrl &mu = missing[i];
        TString szDocUrl;
        int nDocId = mu.nDocId;
        if (nDocId < 0)
            szDocUrl = missingUrls[mu.nDocId];
        else
            szDocUrl = docs[mu.nDocId];
        float fWeight = (float)mu.fWeight;
        if (!reqWeights.empty())
            fWeight *= reqWeights[mu.nReqId];
        toAssess.push_back(SUrlToAssess(queries[mu.nReqId], szDocUrl, fWeight));
    }
    AppendUrls((szLogDir + "/urlsToAssess.txt").c_str(), toAssess);

    {
        TFixedBufferFileOutput f((szLogDir + "/wizQuery.txt").c_str());
        for (size_t i = 0; i < answers.size(); ++i) {
            const SQueryAnswer &ans = answers[i];
            f << ans.query.Text << "\t" << ans.szWizardedQuery.c_str();
            f << "\t" << ans.szCgiParams.c_str() << "\t" << ans.query.Region << "\n";
        }
    }
}
//////////////////////////////////////////////////////////////////////////
int main (int argc, char* argv[])
{
    InitNetworkSubSystem();

    srand(time(nullptr));
#ifdef TEST_AVRG_PRECISION
    //GenerateYaQueries("F:\\!romip\\relevData.txt", "F:\\!romip\\reqData.txt", "C:\\ya\\indexRomip\\yaQueries.txt");
    //TestAvrgPrecision();
#endif

    enum EOp {
        SIMPLE_REQUEST,
        DBG_RELFORM_REQUEST,
        DBG_ATR_REQUEST,
        LARGE_YA_REQUEST,
        SELECTED_REQUEST,
        ATR_SELECTED_REQUEST,
        FORM_SELECTED_URLS,
        ROMIP_ANALYZE,
        FIND_DUPLICATES
    } op = DBG_RELFORM_REQUEST;

    const char* pszAssData = "C:/1/gulin2.txt";
    const char* pszTargetDir = "C:/Code/CalcEp10OnAssess/!";
    szMetaSearchAddress = "tsa.yandex.ru:8039";

    bool bUseHtmlReport = false;
    EFetchType htmlFetch = FT_HTML;
    bool storeMarks = true;

    Opt opt(argc, argv, "ulsSaArdymoh:c:M");
    int optlet;
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
        case 'u':
            op = FORM_SELECTED_URLS;
            break;
        case 'l':
            op = LARGE_YA_REQUEST;
            szMetaSearchAddress = LARGE_YA_XML_SEARCH_HOST;
            break;
        case 's':
            op = DBG_RELFORM_REQUEST;
            break;
        case 'S':
            op = SELECTED_REQUEST;
            break;
        case 'a':
            op = DBG_ATR_REQUEST;
            break;
        case 'A':
            op = ATR_SELECTED_REQUEST;
            break;
        case 'r':
            op = ROMIP_ANALYZE;
            break;
        case 'd':
            op = FIND_DUPLICATES;
            break;
        case 'y':
            op = SIMPLE_REQUEST;
            break;
        case 'm':
            bTakeSingleWordQuery = false;
            break;
        case 'o':
            bTakeMultipleWordQuery = false;
            break;
        case 'h':
            bUseHtmlReport = true;
            szMetaSearchAddress = "yandex.ru";
            htmlFetch = FT_HTML;
            if (strcmp(opt.Arg, "google") == 0) {
                szMetaSearchAddress = "www.google.ru";
                htmlFetch = FT_HTML_GOOGLE;
                bUseSlowMode = true;
            } else if (strcmp(opt.Arg, "yahoo") == 0) {
                szMetaSearchAddress = "yahoo.com";
                htmlFetch = FT_HTML_YAHOO;
                bUseSlowMode = true;
            } else if (strcmp(opt.Arg, "msn") == 0) {
                szMetaSearchAddress = "msn.com";
                htmlFetch = FT_HTML_MSN;
                bUseSlowMode = true;
            } else if (strcmp(opt.Arg, "gogo") == 0) {
                szMetaSearchAddress = "gogo.ru";
                htmlFetch = FT_HTML_GOGO;
                //bUseSlowMode = true;
            }
            break;
        case 'c':
            additionalCgiParams = opt.Arg;
            break;
        case 'M':
            storeMarks = false;
            break;

        default:
            printf("Usage: %s [-ulsSaArdc] assessData targetDir [MetaSearchAddress=tsa.yandex.ru:8039]\n", argv[0]);
            printf("  -c additional cgi params\n");
            printf("  -u form index.selecturls from assess base\n");
            printf("  -l request ya.ru (request is converted to koi-8)\n");
            printf("  -s relform dbgrlv request (default)\n");
            printf("  -S dbgrlv request limited by index.selecturls\n");
            printf("  -a atr dbgrlv request\n");
            printf("  -A atr dbgrlv request limited by index.selecturls\n");
            printf("  -r analyze romip requests results from dolbilka's dir\n");
            printf("  -d find other names for provided URLs\n");
            printf("  -y simple request\n");
            printf("  -m fetch multiple word queries only\n");
            printf("  -o fetch single word queries only\n");
            printf("  -h ya - use html report ( yandex.ru )\n");
            printf("  -h google - use html report ( google.com/ru - depends on server url )\n");
            printf("  -h yahoo - use html report ( yahoo.com )\n");
            printf("  -h msn - use html report ( msn.com )\n");
            printf("  -h gogo - use html report ( gogo.ru )\n");
            return 0;
        }
    }

    if (argc - opt.Ind >= 2) {
        pszAssData = argv[opt.Ind + 0];
        pszTargetDir = argv[opt.Ind + 1];
        if (argc - opt.Ind > 2)
            szMetaSearchAddress = argv[opt.Ind + 2];
    }

    switch (op) {
        case SIMPLE_REQUEST:
        case DBG_RELFORM_REQUEST:
        case DBG_ATR_REQUEST:
        case LARGE_YA_REQUEST:
        case SELECTED_REQUEST:
        case ATR_SELECTED_REQUEST:
            {
                EFetchType ft = FT_SIMPLE;
                bool bOutputDbgRlv = true;
                switch (op) {
                    case SIMPLE_REQUEST: ft = FT_SIMPLE; bOutputDbgRlv = false; break;
                    case DBG_RELFORM_REQUEST: ft = FT_RELFORM_DEBUG; break;
                    case DBG_ATR_REQUEST: ft = FT_ATR_DEBUG; break;
                    case SELECTED_REQUEST: ft = FT_SELECTED; break;
                    case ATR_SELECTED_REQUEST: ft = FT_ATR_SELECTED; break;
                    case LARGE_YA_REQUEST: ft = FT_LARGE_YA; bOutputDbgRlv = false; break;
                    default: assert(0); break;
                }
                if (bUseHtmlReport)
                    ft = htmlFetch;
                TestYandexWithAssessData(pszAssData, pszTargetDir, ft, bOutputDbgRlv, storeMarks);
            }
            break;
        case FORM_SELECTED_URLS:
            GenerateIndexSelectUrls(pszAssData);
            break;
#ifdef _win32_
        case ROMIP_ANALYZE:
            {
                TResultHash estResults, reqResults;
                TVector<float> reqWeights;
                SetCurrentDirectory("C:\\ya\\indexRomip\\");
                ReadOrRelev("F:\\!romip\\orRelevMinusData.txt");
                ReadDolbilkaResults("F:\\!romip\\relevData.txt", 0, "C:\\Work\\Arcadia\\ysite\\dolbilka", &reqResults);
                //TResultHash rq1, rq2;
                //ReadDolbilkaResults("F:\\!romip\\relevData.txt", 0, "C:\\Work\\Arcadia\\ysite\\dolbilka\\1", &rq1);
                //ReadDolbilkaResults("F:\\!romip\\relevData.txt", 0, "C:\\Work\\Arcadia\\ysite\\dolbilka", &rq2);
                //Merge(&reqResults, rq1, rq2);
                LoadRelevData(&estResults, "F:\\!romip\\relevData.txt", true);

                //ReadOrRelev("F:\\!romipHomo\\orRelevMinusData.txt");
                //ReadDolbilkaResults("F:\\!romipHomo\\relevData.txt", 0, "C:\\Work\\Arcadia\\ysite\\dolbilka", &reqResults);
                //LoadRelevData(&estResults, "F:\\!romipHomo\\relevData.txt", true);


                FillRelev(&reqResults, estResults);
                //AnalyzeResults(estResults, reqResults, reqWeights, 0, 0);
                AnalyzeResults(estResults, reqResults, reqWeights, 0, "c:\\ya\\indexRomip\\reqStats.txt");
            }
            break;
        case FIND_DUPLICATES:
            {
                std::ifstream f("C:/!!!/missing.txt");
                TVector<TString> urls, mapped;
                while (!f.eof() && !f.fail()) {
                    char szUrl[10000];
                    f.getline(szUrl, 9800);
                    urls.push_back(szUrl);
                }
                FindDuplicates(urls, &mapped);

                TFixedBufferFileOutput fMap("C:/!!!/mapDuplicate.txt");
                for (size_t i = 0; i < mapped.size(); ++i) {
                    if (!mapped[i].empty())
                        fMap << urls[i].c_str() << "\t" << mapped[i].c_str() << Endl;
                }
            }
            break;
        //case ASSESS_TEXT_ANALYZE:
        //    SetCurrentDirectory("C:\\ya\\indexAssess\\");
        //    ReadDolbilkaResults("F:\\!assess\\relevData.txt", "F:\\!assess\\docDups.txt", "C:\\Work\\Arcadia\\ysite\\dolbilka", &reqResults);
        //    LoadRelevData(&estResults, "F:\\!assess\\relevData.txt", false);
        //    break;
#endif
        default:
            assert(0);
            break;
    }
    return 0;
}
