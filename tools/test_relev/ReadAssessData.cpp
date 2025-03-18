#include "stdafx.h"

#include <ctime>

#include <util/string/cast.h>
#include <util/string/split.h>

#include "ReadAssessData.h"

using namespace std;

void ReadAssessData(const char *pszAssessFileName,
    TVector<TQuery> *queries, TVector<int> *pQueryAges,
    TVector<TString> *pDocs, THashMap<TString,int> *pDocId,
    SHost2Group *pHost2Group,
    TResultHash *pEstimates,
    bool storeMarks)
{
    THashMap<TQuery,int, HashTQueryFunc > queryMap;
    time_t currentTime;
    time(&currentTime);

    TVector<const char*> words;
    TFileInput fData(pszAssessFileName);
    int nLine = 0;
    TString line;
    while (fData.ReadLine(line)) {
        if ((++nLine) % 1000 == 0) {
            printf(".");
            fflush(stdout);
        }
        if (line.empty())
            continue;
        Split((char*)line.data(), '\t', &words);
        if (words.size() != 7)
            ythrow yexception() << "Wrong number of tokens in assess file";
        const char *pszQuery = words[0];
        const char *pszUrl = words[1];

        int nRate = atoi(words[2]);
        TCateg region = FromString<TCateg>(words[4]);
        float fRelev = 0;
        if (nRate == 30)
        {
            // Vital
            fRelev = 0.61f;
        }
        else if (nRate == 20)
        {
            // useful
            fRelev = 0.41f;
        }
        else if (nRate == 10)
        {
            // R+
            fRelev = 0.14f;
        }
        else if (nRate == 5)
        {
            // R-
            fRelev = 0.07f;
        }
        else if (nRate == -10)
        {
            // NotRelev
        }
        else if (nRate == -20 || nRate == -40 || nRate == -50 || nRate == -60)
        {
            fRelev = -10000;
        }

        // detect query id
        int nQueryId;
        {   TQuery q(pszQuery,region);
            THashMap<TQuery,int>::const_iterator i = queryMap.find(q);
            if (i == queryMap.end()) {
                nQueryId = queryMap.size();
                queryMap[q] = nQueryId;
                queries->push_back(q);
                assert(queryMap.size() == queries->size());

                int nYear, nMonth, nDay, nHour, nMinute, nSec;
                sscanf(words[3], "%d-%d-%d %d:%d:%d", &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSec);
                tm timeData;
                memset(&timeData, 0, sizeof(timeData));
                timeData.tm_hour = nHour;
                timeData.tm_min = nMinute - 1;
                timeData.tm_mon = nMonth - 1;
                timeData.tm_year = nYear - 1900;
                timeData.tm_mday = nDay;
                timeData.tm_sec = nSec - 1;
                time_t t = mktime(&timeData);
                double fDays = difftime(currentTime, t) / 3600 / 24;
                pQueryAges->push_back((int)fDays);
            } else
                nQueryId = i->second;
        }

        // detect doc id
        int nDocId;
        {
            THashMap<TString,int>::const_iterator i = pDocId->find(pszUrl);
            if (i == pDocId->end()) {
                nDocId = pDocId->size();
                (*pDocId)[pszUrl] = nDocId;
                pDocs->push_back(pszUrl);
            } else
                nDocId = i->second;
        }

        if (storeMarks) {
            TVector<SReqResult> &dst = (*pEstimates)[nQueryId];
            dst.push_back(SReqResult(nDocId, pHost2Group->GetGroupIdByUrl(pszUrl), 1, fRelev));
        }
    }
    // add estimats with same url
    for (TResultHash::iterator i = pEstimates->begin(); i != pEstimates->end(); ++i) {
        TVector<SReqResult> &res = i->second;
        sort(res.begin(), res.end(), SCmpRRDocId());
        int nDst = -1;
        for (size_t k = 0; k < res.size(); ++k) {
            SReqResult &a = res[k];
            if (nDst >= 0 && res[nDst].DocId == a.DocId) {
                res[nDst].MarkCount += a.MarkCount;
                res[nDst].SumRelev += a.SumRelev;
            } else
                res[++nDst] = a;
        }
        res.resize(nDst + 1);
    }
    printf("\n");
}
