#pragma once

#include "ReadAssessData.h"

#define N_DOWNLOAD_URLS_COUNT 60
#define N_OUTPUT_URLS_COUNT 60

#define FILTER_KNOWN_SPAM

extern bool bTakeSingleWordQuery;
extern bool bTakeMultipleWordQuery;
extern bool bUseSlowMode; // slow mode - use it when get yahoo or msdn reports - to omit ban - 1 request per 20 secs
extern TString additionalCgiParams;

//#define REQUESTS_DECAY_WITH_TIME
#define LARGE_YA_XML_SEARCH_HOST "xmlsearch-p.yandex.ru"

struct SUrlEstimates {
    TString szUrl, szGroup;
    enum {
        N_PARAMS = 11
    };
    ui32 fParams[N_PARAMS]; // rank, TR, LR
    SUrlEstimates() { memset(fParams, 0, sizeof(fParams)); }
    SUrlEstimates(const char *_psz, const char *_pszGroup, ui32 _nRank, ui32 _nTR, ui32 _nLR, ui32 _nPr1,
        ui32 _nPr2, ui32 _nHitWeight, ui32 _nMaxFreq, ui32 _nSumIdf, ui32 _nTRFlags, ui32 _nErfFlags, ui32 _nThemeMatch)
        : szUrl(_psz), szGroup(_pszGroup)
    {
        fParams[0] = _nRank;
        fParams[1] = _nTR;
        fParams[2] = _nLR;
        fParams[3] = _nPr1;
        fParams[4] = _nPr2;
        fParams[5] = _nHitWeight;
        fParams[6] = _nMaxFreq;
        fParams[7] = _nSumIdf;
        fParams[8] = _nTRFlags;
        fParams[9] = _nErfFlags;
        fParams[10] = _nThemeMatch;
    }
};
struct SQueryAnswer {
    int nRequestId;
    TQuery query;
    TString szWizardedQuery, szCgiParams, szFetchUrl;
    TVector<SUrlEstimates> results;
};

extern const char *szMetaSearchAddress;

enum EFetchType {
    FT_SIMPLE,
    FT_RELFORM_DEBUG,
    FT_ATR_DEBUG,
    FT_SELECTED,
    FT_ATR_SELECTED,
    FT_LARGE_YA,
    FT_HTML,
    FT_HTML_GOOGLE,
    FT_HTML_MSN,
    FT_HTML_YAHOO,
    FT_HTML_GOGO
};
void FetchYandex(const TVector<TQuery> &queries, TVector<SQueryAnswer> *pRes, const char *pszDebugOutputDir, EFetchType _fetchType);
