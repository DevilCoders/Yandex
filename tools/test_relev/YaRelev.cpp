#include "stdafx.h"
#include "YaRelev.h"

//////////////////////////////////////////////////////////////////////////
struct SDocOrder {
    TVector<int> order;
};

static void ReadDupData(const char *pszFileName, THashMap<int,int> *pRes) {
    pRes->clear();
    if (!pszFileName)
        return;
    TFileInput f(pszFileName);
    TString line;
    while (f.ReadLine(line)) {
        if (line.empty())
            break;
        int nDoc, nPrevCopy;
        sscanf(line.data(), "%d\t%d", &nDoc, &nPrevCopy);
        (*pRes)[nDoc] = nPrevCopy;
    }
}

static int Normalize(const THashMap<int,int> &dups, int n) {
    THashMap<int,int>::const_iterator i = dups.find(n);
    if (i == dups.end())
        return n;
    return i->second;
}

void ReadDolbilkaResults(const char *pszRelevData, const char *pszDupData, const char *pszDolbilkaRoot, TResultHash *pRes) {
    THashMap<int,int> dups;
    ReadDupData(pszDupData, &dups);

    pRes->clear();
    THashMap<int,bool> seen;

    int nLocalReqId = 0, nFakeGroupId = 0;
    TFileInput fIn(pszRelevData);
    TString line;
    while (fIn.ReadLine(line)) {
        int nDocId, nReqId, nRelevWeight, nRelev;
        sscanf(line.data(), "%d\t%d\t%d\t%d", &nDocId, &nReqId, &nRelevWeight, &nRelev);
        nDocId = Normalize(dups, nDocId);
        if (nDocId == -1 || nReqId == -1)
            continue;
        THashMap<int,bool>::const_iterator seenPtr = seen.find(nReqId);
        if (seenPtr == seen.end()) {
            seen[nReqId];
            char szBuf[1000];
            sprintf(szBuf, "%s\\req%d", pszDolbilkaRoot, nLocalReqId);
            SDocOrder reqRes;
            {
                TFileInput rr(szBuf);
                TString rrLine;
                while (rr.ReadLine(rrLine)) {
                    int n = atoi(rrLine.data());
                    n = Normalize(dups, n);
                    reqRes.order.push_back(n);
                }
            }
            reqRes.order.erase(std::unique(reqRes.order.begin(), reqRes.order.end()), reqRes.order.end());
            TVector<SReqResult> &dst = (*pRes)[nReqId];
            for (size_t i = 0; i < reqRes.order.size(); ++i)
                dst.push_back(SReqResult(reqRes.order[i], nFakeGroupId++, 0, 0));
            ++nLocalReqId;
        }
    }
}

static TVector<string> requests;
static void ReadRequests(const char *pszRequestFileName) {
    requests.clear();
    TFileInput f(pszRequestFileName);
    TString line;
    while (f.ReadLine(line))
        requests.push_back(line.data());
}

void GenerateYaQueries(const char *pszRelevData, const char *pszRequestFileName, const char *pszFileName) {
    ReadRequests(pszRequestFileName);

    TFileInput fIn(pszRelevData);
    TFixedBufferFileOutput fOut(pszFileName);
    THashMap<int,int> seen;
    TString line;
    while (fIn.ReadLine(line)) {
        int nDocId, nReqId, nNotRelev, nRelev;
        sscanf(line.data(), "%d\t%d\t%d\t%d", &nDocId, &nReqId, &nNotRelev, &nRelev);
        if (nDocId == -1 || nReqId == -1)
            continue;
        if (seen.find(nReqId) != seen.end())
            continue;
        seen[nReqId];
        //fOut << "(" << requests[nReqId].c_str() << ")//6\n";
        fOut << requests[nReqId].c_str() << "\n";
    }
}
