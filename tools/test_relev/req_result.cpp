#include "stdafx.h"
#include "req_result.h"

//void Save(const TResultHash &res, const char *pszName) {
//    std::ofstream fOut(pszName);
//    for (TResultHash::const_iterator i = res.begin(); i != res.end(); ++i) {
//        int nReqId = i->first;
//        const TVector<SReqResult> &rVec = i->second;
//        for (int k = 0; k < (int)rVec.size(); ++k) {
//            const SReqResult &r = rVec[k];
//            fOut << nReqId << "\t" << r.nSize << "\t" << r.nDocId << "\t" << r.nRelevWeight << "\t" << r.nRelev << std::endl;
//        }
//    }
//}
//
//void Load(TResultHash *pRes, const char *pszName) {
//    std::ifstream fIn(pszName);
//
//    int nLine = 1;
//    while (!fIn.eof()) {
//        //if (nLine == 17977)
//        //    break;
//        char szBuf[1024];
//        fIn.getline(szBuf, 1000);
//        int nReqId, nLen, nDocId, nRelev, nRelevWeight;
//        sscanf(szBuf, "%d\t%d\t%d\t%d\t%d", &nReqId, &nLen, &nDocId, &nRelevWeight, &nRelev);
//        (*pRes)[nReqId].push_back(SReqResult(nDocId, nLen, nRelev, nRelevWeight));
//        ++nLine;
//    }
//}

void LoadRelevData(TResultHash *pRes, const char *pszName, bool bSubtractRomip) {
    TFileInput fIn(pszName);

    int nLine = 1, nFakeGroupId = 0;
    TString line;
    while (fIn.ReadLine(line)) {
        int nReqId, nDocId, nRelev, nRelevWeight;
        sscanf(line.data(), "%d\t%d\t%d\t%d", &nDocId, &nReqId, &nRelevWeight, &nRelev);
        if (bSubtractRomip) {
            nRelevWeight -= 4;
            nRelev -= 2;
        }
        if (nRelevWeight != 0)
            (*pRes)[nReqId].push_back(SReqResult(nDocId, nFakeGroupId++, 1, nRelev / (nRelevWeight + 8.0f)));
        ++nLine;
    }
}

SReqResult FindEstimate(const TVector<SReqResult> &estimates, int docId, int groupId) {
    for (size_t i = 0; i < estimates.size(); ++i) {
        if (estimates[i].DocId == docId)
            return estimates[i];
    }
    return SReqResult(docId, groupId, 0, 0);
}

void FillRelev(TResultHash *pRes, const TResultHash &estimates) {
    for (TResultHash::iterator i = pRes->begin(); i != pRes->end(); ++i) {
        int nReqId = i->first;
        TVector<SReqResult> &foundDocs = i->second;
        TResultHash::const_iterator k = estimates.find(nReqId);
        if (k == estimates.end())
            continue;
        const TVector<SReqResult> &rqRes = k->second;
        for (size_t k = 0; k < foundDocs.size(); ++k) {
            SReqResult &dst = foundDocs[k];
            int docId = dst.DocId;
            dst = FindEstimate(rqRes, docId, dst.GroupId);
        }
    }
}
