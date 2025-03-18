#pragma once

#include <util/system/yassert.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

struct SReqResult {
    int DocId, GroupId, MarkCount;
    float SumRelev;
    SReqResult() {}
    SReqResult(int _docId, int _groupId, int _markCount, float _relev) : DocId(_docId), GroupId(_groupId), MarkCount(_markCount), SumRelev(_relev) {}

    float GetRelev() const {
        if (MarkCount == 0)
            return 0;
        float f = SumRelev / MarkCount;
        if (f < 0)
            f = -1;
        return f;
    }
};
struct SCmpRR {
    bool operator()(const SReqResult &a, const SReqResult &b) const {
        return a.GetRelev() > b.GetRelev();
    }
};
struct SCmpRRDocId {
    bool operator()(const SReqResult &a, const SReqResult &b) const {
        return a.DocId < b.DocId;
    }
};
typedef THashMap<int, TVector<SReqResult> > TResultHash;
//void Save(const TResultHash &res, const char *pszName);
//void Load(TResultHash *pRes, const char *pszName);
void LoadRelevData(TResultHash *pRes, const char *pszName, bool bSubtractRomip);
SReqResult FindEstimate(const TVector<SReqResult> &estimates, int nDocId, int GroupId);
void FillRelev(TResultHash *pRes, const TResultHash &estimates);

struct SHost2Group {
    THashMap<TString,int> groups;

    int GetGroupId(const TString &szGroup) {
        THashMap<TString,int>::const_iterator i = groups.find(szGroup);
        if (i != groups.end())
            return i->second;
        int nRes = groups.size();
        groups[szGroup] = nRes;
        return nRes;
    }
    TString GetGroup(const TString &szUrl) {
        bool bHttp = szUrl.substr(0,7) == "http://";
        bool bHttps = szUrl.substr(0,8) == "https://";
        if (szUrl == "" || (!bHttp && !bHttps)) {
            if (szUrl != "")
                printf("no group found for url %s\n", szUrl.c_str());
            return ""; // hack
        }
        TString szHost;
        if (bHttp)
            szHost = szUrl.substr(7);
        else if (bHttps)
            szHost = szUrl.substr(8);
        else
            Y_ASSERT(0);
        size_t n = szHost.find('/');
        if (n != szHost.npos)
            szHost = szHost.substr(0, n);
        return szHost;
    }
    // using of this function indicates trouble - potential difference between yandex & this program in grouping
    int GetGroupIdByUrl(const TString &szUrl) {
        TString szGroup = GetGroup(szUrl);
        if (szGroup == "")
            return -1;
        return GetGroupId(szGroup);
        //if (host2group.find(szHost) != host2group.end()) {
        //    TString szWasGroup = host2group[szHost];
        //    ASSERT(szWasGroup == docInfo.szGroup);
        //}
        //host2group[szHost] = docInfo.szGroup;
    }
};
