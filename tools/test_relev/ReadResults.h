#pragma once
#include "FetchYaResults.h"

struct SHost2Group;
void ReadResults(const char *pszPath, const TVector<TString> &queries, TVector<SQueryAnswer> *pRes, SHost2Group *pHost2Group, int nMaxResults);
