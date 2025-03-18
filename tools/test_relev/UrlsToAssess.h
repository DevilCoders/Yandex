#pragma once

#include "ReadAssessData.h"

struct SUrlToAssess {
    TQuery query;
    TString szUrl;
    double fWeight;
    SUrlToAssess() : fWeight(0) {}
    SUrlToAssess(const TQuery& _query, const TString& url, double _fWeight) : query(_query), szUrl(url), fWeight(_fWeight) {}
};
struct SCmpUrlToAssess {
    bool operator()(const SUrlToAssess &a, const SUrlToAssess &b) const { return a.fWeight < b.fWeight; }
};

void AppendUrls(const char *pszFileName, const TVector<SUrlToAssess> &urls);
