#pragma once
#include "semanet_impl.h"
#include "querynet.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
struct ISemanetClassifier {
    virtual ~ISemanetClassifier() {
    }
    virtual void CalcFeatures(const TUtf16String &query, const TVector<TString> &serp, TVector<float> *res) const = 0;
    virtual bool LuckyGuess(const TVector<float> &features) const = 0;
};
ISemanetClassifier* LoadSemanetClassifier(const TString &filesPrefix, TLangMask languages = TLangMask(LANG_RUS, LANG_ENG));
class TBanLists;
////////////////////////////////////////////////////////////////////////////////////////////////////
class TSemanet: public TQueryNet {
public:
    void Load(const TString &filesPrefix);

    struct TBanListOptions {
        TVector<TUtf16String> Queries;
        TVector<TUtf16String> QueryFragments;
        TVector<TString> Hosts;
        TVector<TString> UrlFragments;
        TVector<TUtf16String> WikipediaCategs;
        TVector<TUtf16String> WikipediaCategFragments;
        bool BanPorn;
        bool BanNavigQueries;
        bool BanSuspicious;
        TBanListOptions(bool banDefaultBadQueries);
    };
    struct TOptions {
        size_t MaxQueries;
        size_t MaxHosts;
        size_t MaxUrls;
        bool UseReformulations;
        bool UseUrls;
        bool UseSites;
        float BannedQueryRateThreshold;
        float UrlThreshold;
        float FromUrlProbabilityThreshold;
        float SiteThreshold;
        float FromSiteProbabilityThreshold;
        float HostTooBroadThreshold;
        bool Interactive;
        bool Verbose;
        TLangMask Languages;
        TOptions();
    };
    void TrainClassifier(const TVector<TUtf16String> &seedQueries, const TVector<TString> &seedHosts, const TVector<TString> &seedUrls,
        const TOptions &options, const TBanListOptions &banList, const TBanListOptions &ignoreList, TString outputFilesPrefix) const;

    void RelatedQueries(const TVector<TString> &serp, TVector<TUtf16String> *res, size_t count) const;

    static void Build(TString urlClicksFile, TString shortPersonalizationFile, TString wikipediaPath, TString adultHostsPath, TString suspiciousQueriesPath,
        TString outputPrefix, TLangMask languages = TLangMask(LANG_RUS, LANG_ENG));

private:
    void BuildBanList(const TBanListOptions &banListOpts, TBanLists *res, TString outputFilesPrefix, bool verbose) const;
    void BuildClassifierQueryTrie(const TSemanetTrackers &tracker, const TString &outputFileName, bool verbose) const;
    void BuildClassifierUrlTrie(const TSemanetTrackers &tracker, const TBanLists &banLists, const TString &outputFileName, bool hosts, bool verbose) const;
    void BuildClassifierFragmentsTrie(const TSemanetTrackers &tracker, const TString &outputFileName, bool verbose) const;
    void BuildClassifierWordsTrie(const TSemanetTrackers &tracker, const TString &outputFileName, const TOptions &options) const;
    void PushQueryNeighbors(const TUtf16String &query, const TBanLists &banLists, const TBanLists &ignoreLists, EAddMode &addMode, TSemanetTrackers *res, const TOptions &options, bool force) const;
    void PushUrlNeighbors(const TString &url, const TBanLists &banLists, const TBanLists &ignoreLists, EAddMode &addMode, TSemanetTrackers *res, const TOptions &options, bool force) const;
    void PushHostNeighbors(const TString &host, const TBanLists &banLists, const TBanLists &ignoreLists, EAddMode &addMode, TSemanetTrackers *res, const TOptions &options, bool force) const;
    float CalcHostUrlKnownQueriesFactor(const TSemanetTrackers &tracker, const TBanLists &banLists, const TSemanetRecords &records) const;

    void UrlNeighbors(const TString &url, TResults *res, int maxResultsCount, int maxPathLength, float minProbability) const;
    bool IsNavQuery(const TSemanetRecord &rec, float threshold) const;

    void ReadWikipedia(TString inPath);
    void ReadAdultHosts(TString inPath);

private:
    TSemanetWTrie QueryUrlsTrie;
    TSemanetSTrie UrlsTrie;
    TVector<TString> Urls;
    TSemanetSTrie HostsTrie;
    typedef THashMap<TUtf16String, TVector<TUtf16String> > TWikiCatToQueries;
    TWikiCatToQueries Wiki;
    TVector<TString> AdultHosts;
    typedef TCompactTrie<wchar16, TQueryStats, TAsIsPacker<TQueryStats> > TQueriesTrie;
    TQueriesTrie QueriesTrie;
    typedef TCompactTrie<wchar16, int> TWordsTrie;
    TWordsTrie WordsTrie;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
