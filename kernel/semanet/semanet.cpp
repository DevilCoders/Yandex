#include "semanet.h"
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/charset/wide.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/system/fs.h>
#include <library/cpp/containers/ext_priority_queue/ext_priority_queue.h>
#include <ysite/yandex/doppelgangers/normalize.h>
#include <util/datetime/base.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TTimeTracker {
    TString Description;
    TInstant Start;
    bool Verbose;
    TTimeTracker(const TString &description, bool verbose = true)
        : Description(description)
        , Start(TInstant::Now())
        , Verbose(verbose)
    {
        if (Verbose)
            Cout << description << " started" << Endl;
    }
    ~TTimeTracker() {
        if (Verbose) {
            TDuration timeSpent = TInstant::Now() - Start;
            Cout << Description << " ended, took ";
            ui64 sec = timeSpent.Seconds();
            if (timeSpent > TDuration::Minutes(1))
                Cout << (sec/60) << " min. " << (sec%60) << " sec." << Endl;
            else
                Cout << sec << " sec." << Endl;
        }
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static TString RudeNormalize(TString url) {
    url.to_lower();
    url = CutWWWPrefix(CutHttpPrefix(url));
    if (url.back() == '/')
        url.pop_back();
    return url;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class TBanLists {
    typedef TCompactTrie<wchar16, bool> TTrie;
    typedef TCompactTrieBuilder<wchar16, bool> TBuilder;
    TTrie FullQueries;
    TTrie QueryFragments;
    THashSet<TString> Hosts;
    TVector<TString> UrlFragments;

    TDoppelgangersNormalize Norm;
public:
    bool IsBanned(TStringBuf url) const {
        url = CutWWWPrefix(CutHttpPrefix(url));
        TString host = TString{GetHost(url)};
        host.to_lower();
        if (Hosts.contains(host))
            return true;
        for (size_t n = 0; n < UrlFragments.size(); ++n)
            if (url.find(UrlFragments[n]) != TString::npos)
                return true;
        return false;
    }
    bool IsBanned(TUtf16String query) const {
        query = Norm.Normalize(query, false);
        if (FullQueries.Find(query))
            return true;
        size_t wordStart = 0, wordEnd = query.find(' ', wordStart);
        do {
            size_t prefixLenUnused;
            if (QueryFragments.FindLongestPrefix(query.data() + wordStart, query.length() - wordStart, &prefixLenUnused))
                return true;
            wordStart = wordEnd + 1;
            wordEnd = query.find(' ', wordStart);
        } while (wordStart != 0);
        return false;
    }
private:
    void BuildTrie(const TVector<TUtf16String> &queries, TString saveFileName) {
        TVector<TUtf16String> allQueries;
        for (size_t n = 0; n < queries.size(); ++n) {
            TUtf16String query = Norm.Normalize(queries[n], false);
            allQueries.push_back(query);
        }
        std::sort(allQueries.begin(), allQueries.end());
        TBuilder builder(CTBF_PREFIX_GROUPED);
        for (size_t n = 0; n < allQueries.size(); ++n)
            builder.Add(allQueries[n], true);
        builder.SaveToFile(saveFileName);
    }
public:
    TBanLists()
        : Norm(false, false, false) {
    }
    void BuildQueries(const TVector<TUtf16String> &queries, const TString &saveFilesPrefix) {
        TString saveFileName = saveFilesPrefix + ".banqtrie";
        BuildTrie(queries, saveFileName);
        FullQueries.Init(TBlob::FromFileContent(saveFileName));
    }
    void BuildQueryFragments(const TVector<TUtf16String> &fragments, const TString &saveFilesPrefix) {
        TString saveFileName = saveFilesPrefix + ".banftrie";
        BuildTrie(fragments, saveFileName);
        QueryFragments.Init(TBlob::FromFileContent(saveFileName));
    }
    void AddHost(TStringBuf host) {
        TString realHost = TString{CutWWWPrefix(host)};
        realHost.to_lower();
        Hosts.insert(realHost);
    }
    void AddUrlFragment(const TString &fragment) {
        UrlFragments.push_back(TString{CutWWWPrefix(CutHttpPrefix(fragment))});
    }
    bool IsBanned(const TUtf16String &query, const TVector<TString> &serp) const {
        if (IsBanned(query))
            return true;
        if (serp.empty())
            return false;
        if (IsBanned(serp[0]))
            return true;
        bool ban1 = serp.size() >= 2 && IsBanned(serp[1]);
        bool ban2 = serp.size() >= 3 && IsBanned(serp[2]);
        if (ban1 && ban2)
            return true;
        size_t nBanned = (ban1 || ban2)? 1 : 0;
        for (size_t n = 3; n < serp.size(); ++n)
            if (IsBanned(serp[n]))
                ++nBanned;
        return nBanned >= serp.size() / 2;
    }
    void Save(const TString &saveFilesPrefix) {
        // here we assume that tries were saved already in Build*
        TFixedBufferFileOutput out(saveFilesPrefix + ".banurls");
        for (THashSet<TString>::const_iterator it = Hosts.begin(); it != Hosts.end(); ++it)
            out << "h " << (*it) << Endl;
        for (TVector<TString>::const_iterator it = UrlFragments.begin(); it != UrlFragments.end(); ++it)
            out << "f " << (*it) << Endl;
    }
    void Load(const TString &saveFilesPrefix) {
        FullQueries.Init(TBlob::FromFileContent(saveFilesPrefix + ".banqtrie"));
        QueryFragments.Init(TBlob::FromFileContent(saveFilesPrefix + ".banftrie"));
        TFileInput in(saveFilesPrefix + ".banurls");
        TString line;
        while (in.ReadLine(line)) {
            if (line.empty())
                continue;
            if (line.length() < 3 || line[1] != ' ')
                ythrow yexception() << "Broken ban list format, line: " << line << Endl;
            switch (line[0]) {
                case 'h':
                    Hosts.insert(line.substr(2));
                    break;
                case 'f':
                    UrlFragments.push_back(line.substr(2));
                    break;
            }
        }
    }
    size_t HostCount() const {
        return Hosts.size();
    }
    size_t UrlFragmentsCount() const {
        return UrlFragments.size();
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef TCompactTrie<char, float> TSemanetUClassifierTrie;
typedef TCompactTrieBuilder<char, float> TSemanetUClassifierTrieBuilder;
////////////////////////////////////////////////////////////////////////////////////////////////////
float TSemanet::CalcHostUrlKnownQueriesFactor(const TSemanetTrackers &tracker, const TBanLists &banLists, const TSemanetRecords &records) const {
    float totalP = 0, totalBannedP = 0, totalKnownP = 0;
    for (TSemanetRecords::iterator it = records.begin(); it != records.end(); ++it) {
        const TQueryWithStats& query = Queries[it->TargetId];
        float p = it->Probability;
        bool isBanned = banLists.IsBanned(query.Query);
        totalP += p;
        if (isBanned)
            totalBannedP += p;
        bool isKnown = tracker.Queries.Done.contains(query.Query);
        if (isKnown)
            totalKnownP += p;
    }
    if (totalP == 0 || totalKnownP <= totalBannedP)
        return 0;
    float ret = 20.0f * (totalKnownP - totalBannedP) / totalP;
    if (ret >= 0.99f)
        ret = 0.99f;
    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::BuildClassifierUrlTrie(const TSemanetTrackers &tracker, const TBanLists &banLists, const TString &outputFileName, bool hosts, bool verbose) const {
    const char* message = hosts? "Building classifier hosts trie" : "Building classifier url trie";
    TTimeTracker timer(message, verbose);
    TMap<TString, float> data;

    for (size_t n = 0; n < tracker.Queries.Mined.size(); ++n) {
        TSemanetRecords records;
        QueryUrlsTrie.Find(tracker.Queries.Mined[n], &records);
        for (TSemanetRecords::iterator it = records.begin(); it != records.end(); ++it) {
            const TString &url = Urls[it->TargetId];
            if (banLists.IsBanned(url))
                continue;
            if (hosts) {
                TString host = TString{GetHost(CutWWWPrefix(CutHttpPrefix(url)))};
                data[host] = 0;
            } else {
                data[url] = 0;
            }
        }
    }

    for (TMap<TString, float>::iterator it = data.begin(); it != data.end(); ++it) {
        TSemanetRecords records;
        if (hosts)
            HostsTrie.Find(it->first, &records);
        else
            UrlsTrie.Find(it->first, &records);
        it->second = CalcHostUrlKnownQueriesFactor(tracker, banLists, records);
    }

    const TVector<TString> &seeds = hosts? tracker.Hosts.Mined : tracker.Urls.Mined;
    for (size_t n = 0; n < seeds.size(); ++n)
        data[seeds[n]] = 1.0f;

    TSemanetUClassifierTrieBuilder builder(CTBF_PREFIX_GROUPED);
    for (TMap<TString, float>::const_iterator it = data.begin(); it != data.end(); ++it)
        if (it->second > 0)
            builder.Add(it->first, it->second);
    builder.SaveToFile(outputFileName);
    if (verbose)
        Cout << "Made a trie of " << data.size() << (hosts? " hosts" : " urls") << Endl;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TSemanetClassifierLeaf {
    float Specificity;
    ui8 StepsFromSeed;
    float ProbabilityFromSeed;
    TSemanetClassifierLeaf()
        : Specificity(0)
        , StepsFromSeed(255)
        , ProbabilityFromSeed(0) {
    }
};
typedef TCompactTrie<wchar16, TSemanetClassifierLeaf, TAsIsPacker<TSemanetClassifierLeaf> > TSemanetClassifierTrie;
typedef TCompactTrieBuilder<wchar16, TSemanetClassifierLeaf, TAsIsPacker<TSemanetClassifierLeaf> > TSemanetClassifierTrieBuilder;
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::BuildClassifierQueryTrie(const TSemanetTrackers &tracker, const TString &outputFileName, bool verbose) const {
    TTimeTracker timer("Building classifier query trie", verbose);
    TMap<TUtf16String, TSemanetClassifierLeaf> data;
    for (size_t n = 0; n < tracker.Queries.Mined.size(); ++n) {
        TUtf16String q = tracker.Queries.Mined[n];
        TSemanetClassifierLeaf &dst = data[q];
        dst.StepsFromSeed = 0;
        dst.ProbabilityFromSeed = 1;

        TResults res1;
        QueryNeighbors(q, &res1, 50, 1, 1e-3f);
        for (size_t m = 0; m < res1.size(); ++m) {
            TSemanetClassifierLeaf &dst1 = data[res1[m].Query];
            if (dst1.StepsFromSeed > 1)
                dst1.StepsFromSeed = 1;
            if (!res1[m].PathProbabilities.empty())
                dst1.ProbabilityFromSeed += res1[m].PathProbabilities[0];
        }

        TResults res2;
        QueryNeighbors(q, &res2, 1000, 2, 1e-6f);
        for (size_t m = 0; m < res2.size(); ++m) {
            TSemanetClassifierLeaf &dst2 = data[res2[m].Query];
            if (dst2.StepsFromSeed > 2)
                dst2.StepsFromSeed = 2;
            if (res2[m].PathProbabilities.size() >= 2)
                dst2.ProbabilityFromSeed += res2[m].PathProbabilities[0] * res2[m].PathProbabilities[1];
        }
    }

    TSemanetClassifierTrieBuilder builder(CTBF_PREFIX_GROUPED);
    for (TMap<TUtf16String, TSemanetClassifierLeaf>::iterator it = data.begin(); it != data.end(); ++it) {
        TQueryStats stats;
        QueriesTrie.Find(it->first, &stats);
        if (stats.Popularity == 0)
            it->second.Specificity = 1.0f;
        else
            it->second.Specificity = it->second.ProbabilityFromSeed / stats.Popularity * 1000;
        if (it->second.Specificity > 1)
            it->second.Specificity = 1;
        builder.Add(it->first, it->second);
    }
    builder.SaveToFile(outputFileName);
    if (verbose)
        Cout << "Made a trie of " << data.size() << " queries" << Endl;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef TCompactTrie<wchar16, float> TSemanetFClassifierTrie;
typedef TCompactTrieBuilder<wchar16, float> TSemanetFClassifierTrieBuilder;
////////////////////////////////////////////////////////////////////////////////////////////////////
static void BuildFragmentsTrie(const TMap<TUtf16String, std::pair<int, int> > &counts, TString outputFileName, bool verbose) {
    int added = 0;
    TSemanetFClassifierTrieBuilder builder(CTBF_PREFIX_GROUPED);
    for (TMap<TUtf16String, std::pair<int, int> >::const_iterator it = counts.begin(); it != counts.end(); ++it) {
        int inSeeds = it->second.first, total = it->second.second;
        if (inSeeds < 2)
            continue;
        if (inSeeds == 2 && total > 2)
            continue;
        float weight = inSeeds / (float)total;
        if (weight > 1)
            weight = 1;
        if (inSeeds == 2)
            weight /= 4;
        if (weight < 1e-5f)
            continue;
        if (verbose)
            Cout << "Added a (probably contrast) fragment " << it->first << " with weight " << weight << Endl;
        ++added;
        builder.Add(it->first, weight);
    }
    builder.SaveToFile(outputFileName);
    if (verbose)
        Cout << "Made a trie of " << added << " contrast fragments" << Endl;
}
static void FillWordEnds(const TUtf16String &q, TVector<size_t> *res) {
    size_t wordEnd = 0;
    res->clear();
    do {
        wordEnd = q.find(' ', wordEnd + 1);
        res->push_back(wordEnd);
    } while (wordEnd != TUtf16String::npos);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::BuildClassifierFragmentsTrie(const TSemanetTrackers &tracker, const TString &outputFileName, bool verbose) const {
    TTimeTracker timer("Building classifier fragments trie", verbose);
    TMap<TUtf16String, std::pair<int, int> > counts;

    TVector<size_t> wordEnds;
    for (size_t k = 0; k < tracker.Queries.Mined.size(); ++k) {
        TUtf16String q = tracker.Queries.Mined[k];
        FillWordEnds(q, &wordEnds);
        for (size_t n = 0; n < wordEnds.size(); ++n) {
            size_t wordStart = n? (wordEnds[n-1] + 1) : 0;
            for (size_t m = 0; m < 2 && n + m < wordEnds.size(); ++m) {
                TUtf16String fragment = q.substr(wordStart, wordEnds[n+m] - wordStart);
                ++counts[fragment].first;
            }
        }
    }

    for (size_t k = 0; k < Queries.size(); ++k) {
        TUtf16String q = Queries[k].Query;
        FillWordEnds(q, &wordEnds);
        for (size_t n = 0; n < wordEnds.size(); ++n) {
            size_t wordStart = n? (wordEnds[n-1] + 1) : 0;
            for (size_t m = 0; m < 2 && n + m < wordEnds.size(); ++m) {
                TUtf16String fragment = q.substr(wordStart, wordEnds[n+m] - wordStart);
                if (counts.find(fragment) != counts.end())
                    ++counts[fragment].second;
            }
        }
    }

    BuildFragmentsTrie(counts, outputFileName, verbose);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void ToNormalizedWords(TUtf16String q, TVector<TUtf16String> *res, TLangMask langs) {
    res->clear();
    TokenizeForDoppelgangers(q, res);
    for (size_t n = 0; n < res->size(); ++n)
        (*res)[n] = ToUniqueLemma((*res)[n], langs);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::BuildClassifierWordsTrie(const TSemanetTrackers &tracker, const TString &outputFileName, const TOptions &options) const {
    TTimeTracker timer("Building classifier words trie", options.Verbose);
    TMap<TUtf16String, std::pair<int, int> > counts;

    TVector<TUtf16String> words;
    for (size_t k = 0; k < tracker.Queries.Mined.size(); ++k) {
        ToNormalizedWords(tracker.Queries.Mined[k], &words, options.Languages);
        for (size_t n = 0; n < words.size(); ++n)
            ++counts[words[n]].first;
    }

    for (TMap<TUtf16String, std::pair<int, int> >::iterator it = counts.begin(); it != counts.end(); ++it)
        WordsTrie.Find(it->first, &it->second.second);

    BuildFragmentsTrie(counts, outputFileName, options.Verbose);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
TSemanet::TBanListOptions::TBanListOptions(bool banDefaultBadQueries)
    : BanPorn(banDefaultBadQueries)
    , BanNavigQueries(banDefaultBadQueries)
    , BanSuspicious(false)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////
TSemanet::TOptions::TOptions()
    : MaxQueries(1000)
    , MaxHosts(100)
    , MaxUrls(1000)
    , UseReformulations(true)
    , UseUrls(true)
    , UseSites(true)
    , BannedQueryRateThreshold(0.1f)
    , UrlThreshold(0.3f)
    , FromUrlProbabilityThreshold(0.05f)
    , SiteThreshold(1.0f)
    , FromSiteProbabilityThreshold(0.05f)
    , HostTooBroadThreshold(0.01f)
    , Interactive(false)
    , Verbose(false)
    , Languages(LANG_ENG, LANG_RUS) {
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool HasFragment(TWtringBuf w, const TVector<TUtf16String> &fragments) {
    while (true) {
        for (size_t n = 0; n < fragments.size(); ++n)
            if (w.StartsWith(fragments[n]))
                return true;
        size_t spacePos = w.find(' ');
        if (spacePos == TWtringBuf::npos)
            return false;
        w = w.SubStr(spacePos + 1);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::BuildBanList(const TBanListOptions &banListOpts, TBanLists *res, TString outputFilesPrefix, bool verbose) const {
    for (size_t n = 0; n < banListOpts.Hosts.size(); ++n)
        res->AddHost(banListOpts.Hosts[n]);
    for (size_t n = 0; n < banListOpts.UrlFragments.size(); ++n)
        res->AddUrlFragment(banListOpts.UrlFragments[n]);
    TVector<TUtf16String> allBannedQueries(banListOpts.Queries);
    TVector<TUtf16String> allBannedFragments(banListOpts.QueryFragments);
    if (banListOpts.BanPorn) {
        res->AddUrlFragment("porn");
        res->AddUrlFragment("sex");
        res->AddUrlFragment("seks");
        for (size_t n = 0; n < AdultHosts.size(); ++n)
            res->AddHost(AdultHosts[n]);
        allBannedFragments.push_back(u"porn");
        allBannedFragments.push_back(u"порно");
    }
    if (banListOpts.BanNavigQueries || banListOpts.BanPorn || banListOpts.BanSuspicious) {
        for (size_t n = 0; n < Queries.size(); ++n) {
            int flags = Queries[n].Stats.Flags;
            if (banListOpts.BanPorn && (flags & TQueryStats::FLAG_PORN))
                allBannedQueries.push_back(Queries[n].Query);
            else if (banListOpts.BanNavigQueries && (flags & TQueryStats::FLAG_NAV))
                allBannedQueries.push_back(Queries[n].Query);
            else if (banListOpts.BanSuspicious && (flags & TQueryStats::FLAG_SUSPICIOUS))
                allBannedQueries.push_back(Queries[n].Query);
        }
    }
    TDoppelgangersNormalize norm(false, false, false);
    for (size_t n = 0; n < banListOpts.WikipediaCategs.size(); ++n) {
        TUtf16String cat = norm.Normalize(banListOpts.WikipediaCategs[n], false);
        TWikiCatToQueries::const_iterator it = Wiki.find(cat);
        if (it != Wiki.end())
            for (size_t k = 0; k < it->second.size(); ++k)
                allBannedQueries.push_back(it->second[k]);
    }
    if (!banListOpts.WikipediaCategFragments.empty()) {
        TVector<TUtf16String> normFragments;
        for (size_t n = 0; n < banListOpts.WikipediaCategFragments.size(); ++n) {
            TUtf16String normFragment = norm.Normalize(banListOpts.WikipediaCategFragments[n], false);
            if (!normFragment.empty())
                normFragments.push_back(normFragment);
        }
        size_t wikiQueriesBanned = 0;
        for (TWikiCatToQueries::const_iterator it = Wiki.begin(); it != Wiki.end(); ++it) {
            if (HasFragment(it->first, normFragments)) {
                for (size_t k = 0; k < it->second.size(); ++k) {
                    allBannedQueries.push_back(it->second[k]);
                    ++wikiQueriesBanned;
                }
            }
        }
        if (verbose) {
            Cout << "Banned because of wikipedia categories: " << wikiQueriesBanned << Endl;
            if (wikiQueriesBanned)
                Cout << "Example: " << allBannedQueries.back() << Endl;
        }
    }
    if (verbose) {
        Cout << "Banned " << allBannedQueries.size() << " whole queries" << Endl;
        Cout << "Banned " << allBannedFragments.size() << " query fragments" << Endl;
        Cout << "Banned " << res->HostCount() << " hosts" << Endl;
        Cout << "Banned " << res->UrlFragmentsCount() << " url fragments" << Endl;
    }
    res->BuildQueries(allBannedQueries, outputFilesPrefix);
    res->BuildQueryFragments(allBannedFragments, outputFilesPrefix);
    res->Save(outputFilesPrefix);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
bool GetUserPermission(const char* msg, const T &what, EAddMode &mode) {
    if (mode == ADD_ALWAYS)
        return true;
    if (mode == ADD_NEVER)
        return false;
    TString line;
    while (true) {
        Cout << msg << " " << what << "?    Possible answers: y (yes)/ n (no)/ Y (always yes)/ N (always no)" << Endl;
        Cin.ReadLine(line);
        if (line == "y")
            return true;
        else if (line == "n")
            return false;
        else if (line == "Y") {
            mode = ADD_ALWAYS;
            return true;
        } else if (line == "N") {
            mode = ADD_NEVER;
            return false;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::PushQueryNeighbors(const TUtf16String &query, const TBanLists &banLists, const TBanLists &ignoreLists, EAddMode &addMode,
    TSemanetTrackers *tracker, const TOptions &options, bool force) const
{
    if (tracker->Queries.Done.contains(query) || tracker->Queries.Done.size() >= options.MaxQueries)
        return;
    if (banLists.IsBanned(query) || ignoreLists.IsBanned(query))
        return;
    if (!GetUserPermission("Do you wish to add query ", query, addMode))
        return;
    float totalP = 0, totalBannedP = 0;
    if (options.UseReformulations) {
        TSemanet::TResults results;
        QueryNeighbors(query, &results, 1000, 2, 1e-6f);
        for (size_t k = 0; k < results.size(); ++k) {
            TUtf16String reformulation = results[k].Query;
            float p = 1;
            for (size_t m = 0; m < results[k].PathProbabilities.size(); ++m)
                p *= results[k].PathProbabilities[m];
            bool isBanned = banLists.IsBanned(reformulation);
            totalP += p;
            if (isBanned)
                totalBannedP += p;
            if (!isBanned && !tracker->Queries.Done.contains(reformulation)) {
                tracker->Queries.Inc(reformulation, 1 + p);
            }
        }
    }
    if (options.UseSites || options.UseUrls) {
        TSemanetRecords records;
        QueryUrlsTrie.Find(query, &records);
        for (TSemanetRecords::iterator it = records.begin(); it != records.end(); ++it) {
            const TString &url = Urls[it->TargetId];
            if (banLists.IsBanned(url))
                continue;
            if (options.UseUrls && !tracker->Urls.Done.contains(url))
                tracker->Urls.Inc(url, it->Probability);
            if (options.UseSites) {
                const TString &host = TString{GetHost(CutWWWPrefix(CutHttpPrefix(url)))};
                if (!tracker->Hosts.Done.contains(host))
                    tracker->Hosts.Inc(host, it->Probability);
            }
        }
    }
    tracker->Queries.Done.insert(query);
    if (!force && totalP > 0 && totalBannedP >= totalP * options.BannedQueryRateThreshold) {
        if (options.Verbose)
            Cout << "Not adding query " << query << ": too many banned reformulations (" << (totalBannedP / totalP * 100) << "%)" << Endl;
    } else {
        if (options.Verbose)
            Cout << "Adding query " << query << Endl;
        tracker->Queries.Mined.push_back(query);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::PushUrlNeighbors(const TString &url, const TBanLists &banLists, const TBanLists &ignoreLists, EAddMode &addMode,
    TSemanetTrackers *tracker, const TOptions &options, bool force) const
{
    if (tracker->Urls.Done.contains(url) || tracker->Urls.Done.size() >= options.MaxUrls)
        return;
    if (banLists.IsBanned(url) || ignoreLists.IsBanned(url))
        return;
    if (!GetUserPermission("Do you wish to add url ", url, addMode))
        return;

    float totalP = 0, totalBannedP = 0;
    TSemanetRecords records;
    UrlsTrie.Find(RudeNormalize(url), &records);
    for (TSemanetRecords::iterator it = records.begin(); it != records.end(); ++it) {
        const TQueryWithStats& query = Queries[it->TargetId];
        float p = it->Probability;
        bool isBanned = banLists.IsBanned(query.Query);
        totalP += p;
        if (isBanned)
            totalBannedP += p;
        if (p >= options.FromUrlProbabilityThreshold && !isBanned && !tracker->Queries.Done.contains(query.Query))
            tracker->Queries.Inc(query.Query, 1 + p);
    }

    tracker->Urls.Done.insert(url);
    if (!force && totalP > 0 && totalBannedP >= totalP * options.BannedQueryRateThreshold) {
        if (options.Verbose)
            Cout << "Not adding url " << url << ": too many banned queries (" << (totalBannedP / totalP * 100) << "%)" << Endl;
    } else {
        if (options.Verbose)
            Cout << "Adding url " << url << Endl;
        tracker->Urls.Mined.push_back(url);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::PushHostNeighbors(const TString &host, const TBanLists &banLists, const TBanLists &ignoreLists, EAddMode &addMode,
    TSemanetTrackers *tracker, const TOptions &options, bool force) const
{
    if (tracker->Hosts.Done.contains(host) || tracker->Hosts.Done.size() >= options.MaxHosts)
        return;
    if (banLists.IsBanned(host) || ignoreLists.IsBanned(host))
        return;

    float totalP = 0, totalBannedP = 0, totalKnownP = 0;
    TSemanetRecords records;
    HostsTrie.Find(host, &records);

    TVector<TUtf16String> toAdd;
    TVector<float> toAddP;

    for (TSemanetRecords::iterator it = records.begin(); it != records.end(); ++it) {
        const TQueryWithStats& query = Queries[it->TargetId];
        float p = it->Probability;
        bool isBanned = banLists.IsBanned(query.Query);
        totalP += p;
        if (isBanned)
            totalBannedP += p;
        bool isKnown = tracker->Queries.Done.contains(query.Query);
        if (isKnown)
            totalKnownP += p;
        if (p >= options.FromSiteProbabilityThreshold && !isBanned && !isKnown) {
            toAdd.push_back(query.Query);
            toAddP.push_back(p);
        }
    }

    if (!force && totalKnownP < options.HostTooBroadThreshold * totalP) {
        if (options.Verbose)
            Cout << "Not adding host " << host << ": too broad (specificity is " << (totalKnownP / totalP * 100) << "%)" << Endl;
        return;
    }

    tracker->Hosts.Done.insert(host);
    if (!force && totalP > 0 && totalBannedP >= totalP * options.BannedQueryRateThreshold) {
        if (options.Verbose)
            Cout << "Not adding host " << host << ": too many banned queries (" << (totalBannedP / totalP * 100) << "%)" << Endl;
    } else {
        if (!GetUserPermission("Do you wish to add host ", host, addMode))
            return;
        if (options.Verbose)
            Cout << "Adding host " << host << " (specificity is " << (totalKnownP / totalP * 100) << "%, banned " << (totalBannedP / totalP * 100) << "%)" << Endl;
        for (size_t n = 0; n < toAdd.size(); ++n)
            tracker->Queries.Inc(toAdd[n], 1 + toAddP[n]);
        tracker->Hosts.Mined.push_back(host);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
void WriteTextFile(const T &cont, TString fileName) {
    TFixedBufferFileOutput file(fileName);
    for (typename T::const_iterator it = cont.begin(); it != cont.end(); ++it)
        file << (*it) << Endl;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::TrainClassifier(const TVector<TUtf16String> &seedQueries, const TVector<TString> &seedHosts, const TVector<TString> &seedUrls,
    const TOptions &options, const TBanListOptions &banListOpts, const TBanListOptions &ignoreListOpts, TString outputFilesPrefix) const
{
    TBanLists banLists;
    {
        TTimeTracker tr("Building ban lists", options.Verbose);
        BuildBanList(banListOpts, &banLists, outputFilesPrefix, options.Verbose);
    }
    TBanLists ignoreLists;
    {
        TTimeTracker tr("Building ignore lists", options.Verbose);
        BuildBanList(ignoreListOpts, &ignoreLists, outputFilesPrefix + ".tmp", options.Verbose);
    }

    EAddMode addMode = ADD_ALWAYS;
    TSemanetTrackers tracker;

    for (size_t n = 0; n < seedQueries.size(); ++n)
        PushQueryNeighbors(seedQueries[n], banLists, ignoreLists, addMode, &tracker, options, true);
    for (size_t n = 0; n < seedHosts.size(); ++n)
        PushHostNeighbors(seedHosts[n], banLists, ignoreLists, addMode, &tracker, options, true);
    for (size_t n = 0; n < seedUrls.size(); ++n)
        PushUrlNeighbors(seedUrls[n], banLists, ignoreLists, addMode, &tracker, options, true);

    EAddMode addModeQueries = options.Interactive? ASK_USER : ADD_ALWAYS;
    EAddMode addModeHosts = options.Interactive? ASK_USER : ADD_ALWAYS;
    EAddMode addModeUrls = options.Interactive? ASK_USER : ADD_ALWAYS;
    while (!tracker.Queries.Empty()) {
        TUtf16String q = tracker.Queries.GetMostFrequent();
        PushQueryNeighbors(q, banLists, ignoreLists, addModeQueries, &tracker, options, false);
        if (tracker.Queries.Done.size() >= options.MaxQueries)
            break;
        tracker.Queries.Remove(q);

        bool lookAtHosts = options.UseSites && (tracker.Hosts.Mined.size() < tracker.Queries.Mined.size() * options.MaxHosts / options.MaxQueries);
        if (lookAtHosts) {
            float weight;
            TString host = tracker.Hosts.GetMostFrequent(&weight);
            if (weight > options.SiteThreshold) {
                PushHostNeighbors(host, banLists, ignoreLists, addModeHosts, &tracker, options, false);
                tracker.Hosts.Remove(host);
            }
        }

        bool lookAtUrls = options.UseUrls && (tracker.Urls.Mined.size() < tracker.Queries.Mined.size() * options.MaxUrls / options.MaxQueries);
        if (lookAtUrls) {
            float weight;
            TString url = tracker.Urls.GetMostFrequent(&weight);
            if (weight > options.UrlThreshold) {
                PushUrlNeighbors(url, banLists, ignoreLists, addModeUrls, &tracker, options, false);
                tracker.Urls.Remove(url);
            }
        }
    }

    WriteTextFile(tracker.Queries.Mined, outputFilesPrefix + ".dbg.queries");
    WriteTextFile(tracker.Hosts.Mined, outputFilesPrefix + ".dbg.hosts");
    WriteTextFile(tracker.Urls.Mined, outputFilesPrefix + ".dbg.urls");
    BuildClassifierQueryTrie(tracker, outputFilesPrefix + ".trie", options.Verbose);
    BuildClassifierUrlTrie(tracker, banLists, outputFilesPrefix + ".htrie", true, options.Verbose);
    BuildClassifierUrlTrie(tracker, banLists, outputFilesPrefix + ".utrie", false, options.Verbose);
    BuildClassifierFragmentsTrie(tracker, outputFilesPrefix + ".ftrie", options.Verbose);
    BuildClassifierWordsTrie(tracker, outputFilesPrefix + ".wtrie", options);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::Load(const TString &filesPrefix) {
    TQueryNet::Load(filesPrefix);
    QueryUrlsTrie.Init(TBlob::FromFileContent(filesPrefix + ".qutrie"));
    UrlsTrie.Init(TBlob::FromFileContent(filesPrefix + ".utrie"));
    HostsTrie.Init(TBlob::FromFileContent(filesPrefix + ".htrie"));
    {
        TFileInput ufile(filesPrefix + ".urls");
        TString u;
        while (ufile.ReadLine(u))
            Urls.push_back(u);
    }
    ReadWikipedia(filesPrefix + ".wiki");
    ReadAdultHosts(filesPrefix + ".adult");
    QueriesTrie.Init(TBlob::FromFile(filesPrefix + ".qtrie"));
    WordsTrie.Init(TBlob::FromFile(filesPrefix + ".wtrie"));
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::UrlNeighbors(const TString &url, TResults *res, int maxResultsCount, int maxPathLength, float minProbability) const {
    TSemanetRecords records;
    if (UrlsTrie.Find(RudeNormalize(url), &records))
        Neighbors(records, res, maxResultsCount, maxPathLength, minProbability);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::RelatedQueries(const TVector<TString> &serp, TVector<TUtf16String> *res, size_t count) const {
    TSemanetTracker<TUtf16String> tracker;
    for (size_t n = 0; n < serp.size(); ++n) {
        TSemanet::TResults results;
        UrlNeighbors(serp[n], &results, 200, 2, 1e-6f);
        for (size_t k = 0; k < results.size(); ++k) {
            if (results[k].PathProbabilities.size() < 2)
                continue;
            TUtf16String reformulation = results[k].Query;
            float p = 1;
            for (size_t m = 0; m < results[k].PathProbabilities.size(); ++m)
                p *= results[k].PathProbabilities[m];
            tracker.Inc(reformulation, 1 + p);
        }
    }
    TrackerToResult(&tracker, res, count);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef TCompactTrieBuilder<wchar16, TSemanetRecords, TSemanetRecordsPacker> TSemanetTrieWBuilder;
typedef TCompactTrieBuilder<char,  TSemanetRecords, TSemanetRecordsPacker> TSemanetTrieSBuilder;
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef THashMap<TUtf16String, int> TTopQueries;
static void ReadTopQueries(TString shortPersonalizationFile, TTopQueries *res) {
    TFileInput sp(shortPersonalizationFile);
    TString line;
    while (sp.ReadLine(line)) {
        TVector<TStringBuf> tabs;
        Split(line.begin(), '\t', &tabs);
        int count = FromString<int>(tabs[1]);
        if (count > 500)
            (*res)[UTF8ToWide(tabs[0])] = count;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TQueryStatsAndId {
    int Id;
    int Popularity;
    bool Porn;
    bool Nav;
    bool Suspicious;
    TQueryStatsAndId()
        : Id(-1)
        , Popularity(0)
        , Porn(false)
        , Nav(false)
        , Suspicious(false) {
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
void BuildSemaQueries(TString shortPersonalizationFile, TString outputPrefix, TMap<TUtf16String, TQueryStatsAndId> &queryStats) {
    TTimeTracker tr("Building reformulations trie");

    TTopQueries topQueries;
    ReadTopQueries(shortPersonalizationFile, &topQueries);

    TFileInput sp(shortPersonalizationFile);
    TSemanetTrieWBuilder builder(CTBF_PREFIX_GROUPED);
    TString line;
    TDoppelgangersNormalize norm(false, false, false);
    int numLinesDone = 0;
    while (sp.ReadLine(line)) {
        TVector<TStringBuf> tabs;
        Split(line.begin(), '\t', &tabs);
        TUtf16String w(UTF8ToWide(tabs[0]));
        TUtf16String wNorm = norm.Normalize(w, false);
        int totalCount = FromString<int>(tabs[1]);
        if (totalCount >= 10 && w == wNorm) {
            TQueryStatsAndId &that = queryStats[w];
            that.Popularity = totalCount;
            if (that.Id < 0)
                that.Id = queryStats.size() - 1;
        }
        TVector<TSemanetRecord> recordsToAdd;
        for (size_t n = 2; n < tabs.size(); ++n) {
            TStringBuf tab = tabs[n];
            if (tab.back() == '+')
                tab = tab.SubStr(0, tab.size() - 1);
            size_t semicPos = tab.find(':');
            int count = FromString<int>(tab.SubStr(semicPos + 1));
            if (count < totalCount / 1000)
                continue;
            TUtf16String adjacent(UTF8ToWide(tab.SubStr(0, semicPos)));
            adjacent = norm.Normalize(adjacent, false);
            TTopQueries::const_iterator qit = topQueries.find(adjacent);
            if (qit != topQueries.end() && qit->second > 25 * totalCount)
                continue;
            recordsToAdd.emplace_back();
            float p = count / (float)totalCount;
            if (p > 1)
                p = 1;
            recordsToAdd.back().Probability = p;
            TQueryStatsAndId &stat = queryStats[adjacent];
            if (stat.Id < 0)
                stat.Id = queryStats.size() - 1;
            recordsToAdd.back().TargetId = stat.Id;
        }
        if (!recordsToAdd.empty()) {
            TSemanetRecords reg(&recordsToAdd[0], recordsToAdd.size());
            builder.Add(w, reg);
        }
        ++numLinesDone;
        if ((numLinesDone % 1000000) == 0)
            Cout << "  " << numLinesDone << " lines done" << Endl;
    }
    builder.SaveToFile(outputPrefix + ".reftrie");
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TUrlQueries {
    int Total;
    TVector<std::pair<TUtf16String, int> > Queries;
    TUrlQueries()
        : Total(0) {
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static void BuildUrlTrie(TString outputFileName, TMap<TUtf16String, TQueryStatsAndId> &queryStats, const TMap<TString, TUrlQueries> &map) {
    TSemanetTrieSBuilder builder(CTBF_PREFIX_GROUPED);
    for (TMap<TString, TUrlQueries>::const_iterator it = map.begin(); it != map.end(); ++it) {
        TVector<TSemanetRecord> recordsToAdd;
        float total = it->second.Total;
        for (size_t n = 0; n < it->second.Queries.size(); ++n) {
            TUtf16String query = it->second.Queries[n].first;
            int clicks = it->second.Queries[n].second;
            recordsToAdd.emplace_back();
            float p = clicks / total;
            if (p > 1)
                p = 1;
            recordsToAdd.back().Probability = p;
            recordsToAdd.back().TargetId = queryStats[query].Id;
        }
        TSemanetRecords reg(&recordsToAdd[0], recordsToAdd.size());
        builder.Add(it->first, reg);
    }
    builder.SaveToFile(outputFileName);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsNavQuery(TStringBuf &query, const TString &url, float probability) {
    if (probability > 0.8f && url.find(".wikipedia.org/wiki") == TString::npos)
        return true;
    if ((url == "vk.com" || url == "odnoklassniki.ru") && probability >= 0.5f)
        return true;
    if ((query.StartsWith("http://") || query.StartsWith("www.")) && probability >= 0.5f)
        return true;
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void BuildSemaUrls(TString urlClicksFile, TString outputPrefix, TMap<TUtf16String, TQueryStatsAndId> &queryStats) {
    TTimeTracker tr("Building query-urls trie, urls trie and hosts trie");

    TFileInput uc(urlClicksFile);
    TString line;
    TMap<TString, TUrlQueries> url2queries;
    TMap<TString, TUrlQueries> host2queries;
    TMap<TUtf16String, int> query2popularity;

    { // build .urls and .qutrie
        TUtf16String curQuery;
        TVector<TSemanetRecord> curQueryRecords;
        THashMap<TString, int> urlToId;
        TFixedBufferFileOutput urlsOut(outputPrefix + ".urls");
        TSemanetTrieWBuilder quBuilder(CTBF_PREFIX_GROUPED);
        TDoppelgangersNormalize norm(false, false, false);
        while (uc.ReadLine(line)) {
            TVector<TStringBuf> tabs;
            Split(line.begin(), '\t', &tabs);
            TUtf16String query(UTF8ToWide(tabs[0]));
            TQueryStatsAndId &stat = queryStats[query];
            if (stat.Id < 0)
                stat.Id = queryStats.size() - 1;
            int queryShows = FromString<int>(tabs[1]);
            int queryUrlClicks = FromString<int>(tabs[3]);
            TString url = RudeNormalize(TString{tabs[2]});
            TUrlQueries &dstU = url2queries[url];
            dstU.Total += queryUrlClicks;
            dstU.Queries.push_back(std::make_pair(query, queryUrlClicks));
            TUrlQueries &dstH = host2queries[TString{GetHost(url)}];
            dstH.Total += queryUrlClicks;
            dstH.Queries.push_back(std::make_pair(query, queryUrlClicks));
            if (query != curQuery) {
                if (!curQueryRecords.empty())
                    quBuilder.Add(curQuery, TSemanetRecords(&curQueryRecords[0], curQueryRecords.size()));
                curQuery = query;
                curQueryRecords.clear();
            }
            curQueryRecords.emplace_back();
            int id;
            if (!urlToId.contains(url)) {
                id = urlToId.size();
                urlToId[url] = id;
                urlsOut << url << Endl;
            } else
                id = urlToId[url];
            curQueryRecords.back().TargetId = id;
            float probability = queryUrlClicks / (float)queryShows;
            curQueryRecords.back().Probability = probability;
            if (IsNavQuery(tabs[0], url, probability))
                stat.Nav = true;
        }
        if (!curQueryRecords.empty())
            quBuilder.Add(curQuery, TSemanetRecords(&curQueryRecords[0], curQueryRecords.size()));
        quBuilder.SaveToFile(outputPrefix + ".qutrie");
    }

    BuildUrlTrie(outputPrefix + ".utrie", queryStats, url2queries);
    BuildUrlTrie(outputPrefix + ".htrie", queryStats, host2queries);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static TUtf16String MakeQ(const TDoppelgangersNormalize &norm, TStringBuf q) {
    TUtf16String res = UTF8ToWide(q);
    return norm.Normalize(res, false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::ReadWikipedia(TString inPath) {
    TString line;
    TFileInput in(inPath);
    TVector<TStringBuf> tabs;
    TDoppelgangersNormalize norm(false, false, false);
    while (in.ReadLine(line)) {
        Split(line.begin(), '\t', &tabs);
        if (tabs.size() < 3 || tabs[1] != "Category")
            continue;
        TUtf16String cat = MakeQ(norm, tabs[2]);
        TUtf16String q = MakeQ(norm, tabs[0]);
        if (!cat.empty() && !q.empty())
            Wiki[cat].push_back(q);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::ReadAdultHosts(TString inPath) {
    TString line;
    TFileInput in(inPath);
    while (in.ReadLine(line)) {
        size_t tabPos = line.find('\t');
        AdultHosts.push_back(RudeNormalize(line.substr(0, tabPos)));
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void AssignAdultQueriesFlag(TString filePath, TMap<TUtf16String, TQueryStatsAndId> &queryData) {
    TTimeTracker tr("Assigning flags to queries");

    TFileInput in(filePath);
    TString line;
    TDoppelgangersNormalize norm(false, false, false);
    TVector<TStringBuf> tabs;
    while (in.ReadLine(line)) {
        Split(line.begin(), '\t', &tabs);
        TUtf16String query = norm.Normalize(UTF8ToWide(tabs[0]), false);
        TMap<TUtf16String, TQueryStatsAndId>::iterator it = queryData.find(query);
        if (it == queryData.end())
            continue;
        if (tabs[2] == "porno_pref_url" || tabs[2] == "porno_pref_url_except")
            continue;
        it->second.Suspicious = true;
        if (tabs[2].find("porno") != TStringBuf::npos)
            it->second.Porn = true;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSemanet::Build(TString urlClicksFile, TString shortPersonalizationFile, TString wikipediaPath, TString adultHostsPath, TString suspiciousQueriesPath,
    TString outputPrefix, TLangMask languages)
{
    TMap<TUtf16String, TQueryStatsAndId> queryStats;

    BuildSemaQueries(shortPersonalizationFile, outputPrefix, queryStats);
    BuildSemaUrls(urlClicksFile, outputPrefix, queryStats);
    AssignAdultQueriesFlag(suspiciousQueriesPath, queryStats);

    Cout << "Dumping queries list" << Endl;
    TVector<TUtf16String> queries(queryStats.size());
    TVector<TQueryStatsAndId> queryStatsVec(queryStats.size());
    for (TMap<TUtf16String, TQueryStatsAndId>::const_iterator it = queryStats.begin(); it != queryStats.end(); ++it) {
        queries[it->second.Id] = it->first;
        queryStatsVec[it->second.Id] = it->second;
    }
    TFixedBufferFileOutput out(outputPrefix + ".queries");
    for (size_t n = 0; n < queries.size(); ++n) {
        out << queries[n] << "\t" << queryStatsVec[n].Popularity;
        if (queryStatsVec[n].Porn)
            out << "\tporn";
        if (queryStatsVec[n].Nav)
            out << "\tnav";
        if (queryStatsVec[n].Suspicious)
            out << "\tsusp";
        out << Endl;
    }

    Cout << "Building adult hosts and wikipedia parts" << Endl; // in fact, now we do nothing other than copy them, but later we may add some preprocessing
    NFs::Copy(adultHostsPath, outputPrefix + ".adult");
    NFs::Copy(wikipediaPath, outputPrefix + ".wiki");

    Cout << "Building qtrie" << Endl;
    TCompactTrieBuilder<wchar16, TQueryStats, TAsIsPacker<TQueryStats> > builder(CTBF_PREFIX_GROUPED);
    for (TMap<TUtf16String, TQueryStatsAndId>::const_iterator it = queryStats.begin(); it != queryStats.end(); ++it) {
        TQueryStats toAdd;
        toAdd.Popularity = it->second.Popularity;
        if (it->second.Porn)
            toAdd.Flags |= TQueryStats::FLAG_PORN;
        if (it->second.Nav)
            toAdd.Flags |= TQueryStats::FLAG_NAV;
        if (it->second.Suspicious)
            toAdd.Flags |= TQueryStats::FLAG_SUSPICIOUS;
        builder.Add(it->first, toAdd);
    }
    builder.SaveToFile(outputPrefix + ".qtrie");

    Cout << "Building words trie" << Endl;
    TMap<TUtf16String, int> word2count;
    TVector<TUtf16String> words;
    for (size_t n = 0; n < queries.size(); ++n) {
        ToNormalizedWords(queries[n], &words, languages);
        for (size_t m = 0; m < words.size(); ++m)
            ++word2count[words[m]];
        if ((n % 100000) == 0)
            Cout << "  words data for " << n << " queries calculated" << Endl;
    }
    TCompactTrieBuilder<wchar16, int> wordTrieBuilder(CTBF_PREFIX_GROUPED);
    for (TMap<TUtf16String, int>::const_iterator it = word2count.begin(); it != word2count.end(); ++it) {
        if (it->second > 1)
            wordTrieBuilder.Add(it->first, it->second);
    }
    wordTrieBuilder.SaveToFile(outputPrefix + ".wtrie");

}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TSemanetClassifier: public ISemanetClassifier {
    TBanLists BanLists;
    TSemanetClassifierTrie Trie;
    TSemanetUClassifierTrie HTrie;
    TSemanetUClassifierTrie UTrie;
    TSemanetFClassifierTrie FTrie;
    TSemanetFClassifierTrie WTrie;
    TDoppelgangersNormalize Norm;
    TLangMask Languages;

    TSemanetClassifier(const TString &prefix, TLangMask languages)
        : Norm(false, false, false)
        , Languages(languages)
    {
        BanLists.Load(prefix);
        Trie.Init(TBlob::FromFileContent(prefix + ".trie"));
        HTrie.Init(TBlob::FromFileContent(prefix + ".htrie"));
        UTrie.Init(TBlob::FromFileContent(prefix + ".utrie"));
        FTrie.Init(TBlob::FromFileContent(prefix + ".ftrie"));
        WTrie.Init(TBlob::FromFileContent(prefix + ".wtrie"));
    }
    void CalcFeatures(const TUtf16String &query, const TVector<TString> &serp, TVector<float> *res) const override {
        TVector<float> &features = *res;
        features.assign(11, 0.0f);

        TUtf16String normQuery = Norm.Normalize(query, false);

        // is it banned?
        features[0] = BanLists.IsBanned(normQuery, serp);

        // features for known queries
        TSemanetClassifierLeaf leaf;
        Trie.Find(normQuery, &leaf);
        features[1] = leaf.Specificity;
        features[2] = leaf.StepsFromSeed;
        features[3] = leaf.ProbabilityFromSeed;

        // is it an expansion of a known query?
        features[4] = features[2];
        TVector<TSemanetClassifierTrie::TPhraseMatch> matches;
        Trie.FindPhrases(normQuery, matches);
        for (size_t n = 0; n < matches.size(); ++n)
            if (matches[n].second.StepsFromSeed < features[4])
                features[4] = matches[n].second.StepsFromSeed;

        // SERP features
        if (!serp.empty()) {
            int totalSeedHosts = 0, totalSeedUrls = 0;
            float totalHostsP = 0, totalUrlP = 0;
            for (size_t n = 0; n < serp.size(); ++n) {
                TString url = RudeNormalize(serp[n]);
                TString host = TString{GetHost(url)};
                float pU, pH;
                if (HTrie.Find(host, &pH)) {
                    totalHostsP += pH;
                    if (pH > 0.99f)
                        totalSeedHosts += pH;
                }
                if (UTrie.Find(url, &pU)) {
                    totalUrlP += pU;
                    if (pU > 0.99f)
                        totalSeedUrls += pU;
                }
            }
            float size = serp.size();
            features[5] = totalSeedUrls / size;
            features[6] = totalSeedHosts / size;
            features[7] = totalUrlP / size;
            features[8] = totalHostsP / size;
        }

        // Contrast fragment feature
        TVector<TSemanetFClassifierTrie::TPhraseMatch> fragmentMatches;
        size_t wordStart = 0;
        do {
            FTrie.FindPhrases(normQuery.data() + wordStart, normQuery.size() - wordStart, fragmentMatches);
            for (size_t n = 0; n < fragmentMatches.size(); ++n) {
                float contrast = fragmentMatches[n].second;
                if (features[9] < contrast)
                    features[9] = contrast;
            }
            wordStart = normQuery.find(' ', wordStart) + 1;
        } while (wordStart != 0);

        // Contrast words feature
        TVector<TUtf16String> words;
        ToNormalizedWords(normQuery, &words, Languages);
        for (size_t n = 0; n < words.size(); ++n) {
            float contrast = 0;
            if (FTrie.Find(words[n], &contrast) && features[10] < contrast)
                features[10] = contrast;
        }
    }
    bool LuckyGuess(const TVector<float> &features) const override {
        if (features.size() < 10)
            ythrow yexception() << "Bad usage of LuckyGuess: features should be calculated first" << Endl;
        if (features[0] > 0)
            return false;
        if (features[2] == 0)
            return true;
        if (features[1] > 1e-4f)
            return true;
        if (features[1] > 1e-5f && (features[5] > 0.2f || features[6] > 0.5f))
            return true;
        if (features[5] > 0.6f || features[6] >= 0.8f)
            return true;
        return false;
    }
};
ISemanetClassifier* LoadSemanetClassifier(const TString &filesPrefix, TLangMask languages) {
    return new TSemanetClassifier(filesPrefix, languages);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
