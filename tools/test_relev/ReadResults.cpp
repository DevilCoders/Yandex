#include "StdAfx.h"
#include "ReadResults.h"
#include "StrUtil.h"
#include "req_result.h"

void ReadResults(const char *pszPath, const TVector<TString> &queries, TVector<SQueryAnswer> *pRes, SHost2Group *pHost2Group, int nMaxResults) {
    pRes->resize(0);
    TVector<const char*> words;
    for (size_t i = 0; i < queries.size(); ++i) {
        char szFileName[1024];
        sprintf(szFileName, "%s/req%d", pszPath, i);
        std::ifstream f(szFileName);
        SQueryAnswer &qa = *pRes->insert(pRes->end(), SQueryAnswer());
        qa.nRequestId = i;
        qa.szQuery = queries[i];
        while (!f.eof() && !f.fail()) {
            char szLine[10240];
            f.getline(szLine, 10000);
            Split(szLine, &words);
            if (words.size() != 1)
                break;
            // should not use GetGroup()
            qa.results.push_back(SUrlEstimates(words[0], pHost2Group->GetGroup(words[0]).c_str(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
        }
        if (qa.results.size() > (size_t)nMaxResults)
            qa.results.resize(nMaxResults);
    }
}
