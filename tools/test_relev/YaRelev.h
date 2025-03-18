#pragma once
#include "req_result.h"

void GenerateYaQueries(const char *pszRelevData, const char *pszRequestFileName, const char *pszFileName);
void ReadDolbilkaResults(const char *pszRelevData, const char *pszDupData, const char *pszDolbilkaRoot, TResultHash *pRes);
