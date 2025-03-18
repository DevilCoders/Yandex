#include "stdafx.h"

#include <fstream>

#include <util/generic/string.h>

#include "UrlsToAssess.h"

struct SAUHost {
    TString szHost;
    double fWeight;
};
struct SAUHostCmp {
    bool operator()(const SAUHost &a, const SAUHost &b) const {
        return a.fWeight > b.fWeight;
    }
};

void AppendUrls(const char *pszFileName, const TVector<SUrlToAssess> &urls) {
    {
        std::ofstream f(pszFileName, std::ios::app);
        for (size_t i = 0; i < urls.size(); ++i) {
            const SUrlToAssess &u = urls[i];
            f << u.query.Text << "\t" << u.szUrl << "\t" << u.fWeight << "\t" << hash<const char*>()(u.szUrl.c_str()) << "\t" << u.query.Region << std::endl;
        }
    }

    THashMap<TString,TVector<SUrlToAssess> > groupByHost;
    for (size_t i = 0; i < urls.size(); ++i) {
        const SUrlToAssess &u = urls[i];
        TString szHost = u.szUrl;
        szHost = szHost.substr(7);
        size_t n = szHost.find('/');
        if (n != TString::npos)
            szHost = szHost.substr(0, n);
        groupByHost[szHost].push_back(u);
    }
    TVector<SAUHost> hosts;
    for (THashMap<TString,TVector<SUrlToAssess> >::const_iterator i = groupByHost.begin(); i != groupByHost.end(); ++i) {
        const TVector<SUrlToAssess> &h = i->second;
        double fTotal = 0;
        for (size_t k = 0; k < h.size(); ++k)
            fTotal += h[k].fWeight;
        SAUHost hRes;
        hRes.szHost = i->first;
        hRes.fWeight = fTotal;
        hosts.push_back(hRes);
    }
    std::sort(hosts.begin(), hosts.end(), SAUHostCmp());
    std::ofstream fHost((TString(pszFileName) + "_byhost").c_str());
    for (size_t i = 0; i < hosts.size(); ++i) {
        const SAUHost &hst = hosts[i];
        fHost << hst.szHost.c_str() << "\t" << hst.fWeight << std::endl;
        TVector<SUrlToAssess> h = groupByHost[hst.szHost];
        std::sort(h.begin(), h.end(), SCmpUrlToAssess());
        for (size_t k = 0; k < h.size(); ++k)
            fHost << "\t" << h[k].query.Text << "\t" << h[k].szUrl << "\t" << h[k].fWeight << "\t" << h[k].query.Region << std::endl;
    }
}

struct SCmpByQueryAndUrl {
    bool operator()(const SUrlToAssess &a, const SUrlToAssess &b) const {
        if (a.query < b.query)
            return true;
        if (a.query == b.query)
            return a.szUrl < b.szUrl;
        return false;
    }
};
