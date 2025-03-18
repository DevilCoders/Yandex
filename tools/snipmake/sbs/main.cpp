#include <util/random/shuffle.h>
#include <util/system/env.h>
#include <quality/deprecated/Misc/commonStdAfx.h>
#include <mapreduce/lib/init.h>
#include <mapreduce/interface/table.h>
#include <mapreduce/interface/update.h>
#include <mapreduce/interface/client.h>
#include <mapreduce/library/temptable/temptable.h>
#include <mapreduce/library/multiset_model/multiset_model.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/getopt/opt.h>
#include <library/cpp/containers/ext_priority_queue/ext_priority_queue.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <quality/user_sessions/request_aggregate_lib/all.h>
#include <util/random/random.h>
#include <util/thread/pool.h>
#include <quality/trailer/suggest/fastcgi_lib/http/get.h>

using namespace NMR;

////////////////////////////////////////////////////////////////////////////////////////////////////
static TString PrepareUrl(TStringBuf url) {
    // cut prefixes
    TStringBuf noPrefix = CutWWWPrefix(CutHttpPrefix(url));
    // host to lower
    size_t hostEnd = noPrefix.find('/');
    TString host = TString{noPrefix.SubStr(0, hostEnd)};
    host.to_lower();
    // recode CP1251 in url to fix wikipedia doubles
    TString urlOnly = TString{noPrefix.SubStr(hostEnd)};
    UrlUnescape(urlOnly);
    if (!IsUtf(urlOnly))
        urlOnly = WideToUTF8(CharToWide(urlOnly, CODES_WIN));
    // remove trailing slash
    if (!urlOnly.empty() && urlOnly.back() == '/')
        urlOnly.pop_back();
    return host + urlOnly;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// "tech" wizards, actual list is located somewhere in report
static const TVector<TString> NonStrataWizards = {
"lang_hint", "minuswords", "minuswords_obsolete", "misspell_source",
"unquote", "pi_e", "web_misspell", "switch_off_thes", "request_filter",
"anti_pirate", "navigation_context", "misspell", "stripe", "reask", "auto", "pseudo", "rasp", "sms", "topic", "object-answer-badge"};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TQueryRegion {
    TString Query;
    TString Region;

    TQueryRegion(const TString& query, const TString& region)
        : Query(query)
        , Region(region)
    {
    }

    TQueryRegion(const TString& queryRegion, int defaultRegion = 0)
    {
        size_t pos = queryRegion.find('\t');
        if (pos == TString::npos) {
            Query = queryRegion;
            Region = ToString<int>(defaultRegion);
        } else {
            Query = queryRegion.substr(0, pos);
            Region = queryRegion.substr(pos + 1);
        }
    }

    bool operator==(const TQueryRegion& other) const
    {
        return Query == other.Query;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TStrata {
    int Count;
    TExtPriorityQueue<TQueryRegion> Reqs;
    TStrata()
        : Count(0) {
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class TSelectFeatureStratified: public IReduce {
    OBJECT_METHODS(TSelectFeatureStratified);
    int Region;
    TString Domain;
    TString Uil;
    bool BanPorn;
    bool AddSerps;
    TVector<TString> AllowedStrata;
    THashMap<TString, TString> UrlStrata;
    mutable THashMap<TString, TStrata> Strata;
    mutable int TotalCount = 0;
private:
    int operator& (IBinSaver &f) override { f.Add(0, &Region); f.Add(0, &Domain); f.Add(0, &Uil); f.Add(0, &BanPorn); f.Add(0, &AddSerps); f.Add(0, &UrlStrata); f.Add(0, &AllowedStrata); return 0; }

    TString GetStrataName(const NRA::TYandexWebRequest* r, TString *query) const {
        const NRA::TBlocks &blocks = r->GetMainBlocks();
        if (AddSerps) {
            for (NRA::TBlocks::const_iterator itBlock = blocks.begin(); itBlock != blocks.end(); ++itBlock) {
                const NRA::TBlock &b = **itBlock;
                const NRA::TResult* mainResult = b.GetMainResult();
                if (const NRA::TWebResult* webRes = dynamic_cast<const NRA::TWebResult*>(mainResult)) {
                    *query += " | " + webRes->GetUrl();
                }
            }
            if (query->length() > 4000)
                query->resize(4000);
        }
        for (NRA::TBlocks::const_iterator itBlock = blocks.begin(); itBlock != blocks.end(); ++itBlock) {
            const NRA::TBlock &b = **itBlock;
            const NRA::TResult* mainResult = b.GetMainResult();
            if (const NRA::TWizardResultProperties* wizRes = dynamic_cast<const NRA::TWizardResultProperties*>(mainResult)) {
                if (!IsIn(NonStrataWizards, wizRes->GetName()))
                    return wizRes->GetName();
            }
            if (const NRA::TWebResult* webRes = dynamic_cast<const NRA::TWebResult*>(mainResult)) {
                if (!UrlStrata.empty()) {
                    auto it = UrlStrata.find(PrepareUrl(webRes->GetUrl()));
                    if (it != UrlStrata.end())
                        return it->second;
                }
                if (webRes->GetNavLevel() > NL_USUAL)
                    return "nav";
                TStringBuf host = GetHost(CutHttpPrefix(webRes->GetUrl()));
                if (IsIn(AllowedStrata, "wiki") && host.EndsWith("wikipedia.org"))
                    return "wiki";
                if (IsIn(AllowedStrata, "kino") && host.EndsWith("kinopoisk.ru"))
                    return "kino";
                if (IsIn(AllowedStrata, "auto") && host.EndsWith("auto.ru"))
                    return "kino";
            }
        }
        return "web";
    }

public:
    TSelectFeatureStratified() {
    }
    TSelectFeatureStratified(int region, TString domain, TString uil, bool banPorn, bool addSerps, const TVector<TString> &allowedStrata, const THashMap<TString, TString> &urlStrata)
        : Region(region)
        , Domain(domain)
        , Uil(uil)
        , BanPorn(banPorn)
        , AddSerps(addSerps)
        , AllowedStrata(allowedStrata)
        , UrlStrata(urlStrata)
    {
    }
    void Do(TValue, TTableIterator &input, TUpdate&) override
    {
        NRA::TLogsParser lp(nullptr);
        for (; input.IsValid(); ++input) {
            lp.AddRec(input.GetKey(), input.GetSubKey(), input.GetValue());
            if (lp.IsFatUser()) {
                return;
            }
        }
        lp.Join();
        NRA::TRequestsContainer rcont = lp.GetRequestsContainer();
        const NRA::TRequests &requests = rcont.GetRequests();
        NRA::TFilterPack fpack;
        if (!Domain.empty()) {
            TVector<TString> domains(1, Domain);
            fpack.Add(new NRA::TDomainFilter(domains));
            fpack.Init();
        }
        for (auto r : requests) {
            NRA::TYandexWebRequest *yr = dynamic_cast<NRA::TYandexWebRequest*>(r.Get());
            if (!yr)
                continue;
            int userRegion = yr->GetUserRegion();
            if (!Domain.empty()) {
                if (!fpack.Filter(*yr))
                    continue;
            } else {
                if (Region != userRegion)
                    continue;
            }
            if (!Uil.empty() && yr->GetUILanguage() != Uil)
                continue;
            TString query = yr->GetQuery();
            if (query.empty())
                continue;

            if (BanPorn) {
                TString sp = yr->GetSearchProps();
                if (sp.find("Porno.pl=12") != TString::npos || sp.find("Porno.pl=100") != TString::npos)
                    continue;
                if (yr->GetMSP().Misspell || yr->GetReask().Misspell)
                    continue;
                if (query.find("порн") != TString::npos) // if everything else failed...
                    continue;
            }
            TString strataName = GetStrataName(yr, &query);
            if (AllowedStrata.empty() || IsIn(AllowedStrata, strataName)) {
                TStrata &dst = Strata[strataName];
                ++dst.Count;
                dst.Reqs.push(TQueryRegion(query, ToString<int>(userRegion)), RandomNumber<ui32>());
                if (dst.Reqs.size() > 100)
                    dst.Reqs.pop();
            }
            ++TotalCount;
        }
    }
    void Finish(ui32, TUpdate& output) override {
        for (auto it : Strata) {
            TString name = it.first;
            TStrata &val = it.second;
            output.AddSub(name, "*", ToString(val.Count));
            while (!val.Reqs.empty()) {
                output.AddSub(name, ToString(val.Reqs.top().Pri), val.Reqs.ytop().Query + "\t" + val.Reqs.ytop().Region);
                val.Reqs.pop();
            }
        }
        output.AddSub("*", "*", ToString(TotalCount));
    }
};
REGISTER_SAVELOAD_CLASS(0x01, TSelectFeatureStratified);
////////////////////////////////////////////////////////////////////////////////
struct TBeta {
    TString BetaName;
    TString ExpFlags;
    TVector<TString> Deletions;

    TBeta() {
    }

    TBeta(TString opt) {
        if (opt.StartsWith("http://"))
            BetaName = opt;
        else
            ExpFlags = opt;
    }
};
////////////////////////////////////////////////////////////////////////////////
struct TDownload {
    TBeta Beta;
    TString EscapedQuery;
    TString Region;
    TString OutputFileName;

    TDownload(const TBeta &beta, TString escapedQuery, TString region, TString fileName)
        : Beta(beta)
        , EscapedQuery(escapedQuery)
        , Region(region)
        , OutputFileName(fileName)
    {
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static int CutFromHtml(TString *html, TString tagName, TString tagWithParams)
{
    TString cutResult;
    TString openTag = "<" + tagName;
    TString closeTag = "</" + tagName + ">";
    size_t from = 0;
    int cutCount = 0;
    while (true) {
        size_t to = html->find(tagWithParams, from);
        cutResult += html->substr(from, to - from);
        if (to == TString::npos)
            break;
        ++cutCount;
        int stackCount = 0;
        while (true) {
            size_t openPos = html->find(openTag, to + 1);
            size_t closePos = html->find(closeTag, to + 1);
            if (openPos < closePos) {
                ++stackCount;
                to = openPos;
            } else if (stackCount > 0) {
                --stackCount;
                to = closePos + 1;
            } else {
                from = closePos + closeTag.length();
                break;
            }
        }
    }
    *html = cutResult;
    return cutCount;
}

////////////////////////////////////////////////////////////////////////////////
bool DownloadAndDump(const TDownload &download, size_t timeout, THashMap<TString, TString> &downloadResults)
{
    TString url = download.Beta.BetaName + download.EscapedQuery + download.Beta.ExpFlags + "&lr=" + download.Region;
    TString response;
    bool success = NFastCgiLib::TryGetHttpResponseFollowRedirs(url, false, timeout, &response);
    if (success) {
        for (TString del: download.Beta.Deletions) {
            CutFromHtml(&response, "div", "<div " + del);
        }
        TFixedBufferFileOutput out1(download.OutputFileName);
        out1 << response;

        static TMutex lock;
        TGuard<TMutex> guard(lock);
        downloadResults[download.OutputFileName] = response;
    } else {
        Cerr << "curl \"" << url << "\" >> " << download.OutputFileName << Endl;
    }
    return success;
}

////////////////////////////////////////////////////////////////////////////////
static void SelectStratified(THashMap<TString, TStrata> strata, int maxCountToSelect, int totalSelectedStrataCount, TVector<TQueryRegion> *res) {
    res->clear();
    Cerr << "Trying to select a stratified sample of at most " << maxCountToSelect << Endl;
    for (auto it : strata) {
        TStrata &s = it.second;
        double strataRatio = (double)s.Count / totalSelectedStrataCount;
        int countToSelectFromStrata = int(strataRatio * maxCountToSelect);
        if (countToSelectFromStrata) {
            Cerr << "Dumping " << countToSelectFromStrata << " samples of " << it.first << Endl;
            for (int n = 0; n < countToSelectFromStrata; ++n) {
                if (s.Reqs.empty())
                    break;
                auto queryRegion = s.Reqs.ytop();
                if (!IsIn(*res, queryRegion))
                    res->push_back(queryRegion);
                s.Reqs.pop();
            }
        }
    }
    Cerr << "As a result, selected " << res->size() << Endl;
}
////////////////////////////////////////////////////////////////////////////////
struct TPlannedPair {
    TString Query;
    TString Region;
    TString Left, Right;
    int LeftN, RightN;
};
////////////////////////////////////////////////////////////////////////////////
static size_t GetNeededQueriesAmount(const TVector<TBeta>& betas, int cross, bool withProductionOnly) {
    return withProductionOnly ? 2 * (betas.size() - 1) * cross : betas.size() * (betas.size() - 1) * cross;
}
////////////////////////////////////////////////////////////////////////////////
void DoFinalAction(TVector<TQueryRegion> &queries, int cross, TVector<TBeta> betas, int, TString prefix, TString action, size_t timeout, size_t threadsCount, bool withProductionOnly, int times) {
    if (queries.size() < GetNeededQueriesAmount(betas, cross, withProductionOnly))
        ythrow yexception() << "Too little amount of queries";

    Shuffle(queries.begin(), queries.end());

    if (action == "list") {
        for (auto q : queries)
            Cout << q.Query << "\t" << q.Region << Endl;
        return;
    }

    int current = 0;

    TVector<TPlannedPair> plan;
    TVector<TDownload> downloads;
    for (int c = 0; c < cross; ++c) {
        for (size_t n = 0; n < betas.size(); ++n) {
            for (size_t m = 0; m < betas.size(); ++m) {
                if (n != m) {
                    if (withProductionOnly && n + 1 != betas.size() && m + 1 != betas.size()) {
                        continue;
                    }
                    TPlannedPair pair;
                    pair.Query = queries[current].Query;
                    pair.Region = queries[current].Region;
                    pair.LeftN = n;
                    pair.RightN = m;
                    TString escapedQuery = pair.Query;
                    CGIEscape(escapedQuery);
                    if (action == "download") {
                        for (int i = 0; i < times; ++i) {
                            pair.Left = "test" + ToString(current + 1) + "_l.html";
                            pair.Right = "test" + ToString(current + 1) + "_r.html";
                            if (times > 1) {
                                pair.Left += "_v" + ToString(i + 1);
                                pair.Right += "_v" + ToString(i + 1);
                            }
                            downloads.push_back(TDownload(betas[n], escapedQuery, pair.Region, pair.Left));
                            downloads.push_back(TDownload(betas[m], escapedQuery, pair.Region, pair.Right));
                        }
                    } else {
                        pair.Left = betas[n].BetaName + escapedQuery + betas[n].ExpFlags + "&lr=" + pair.Region;
                        pair.Right = betas[m].BetaName + escapedQuery + betas[m].ExpFlags + "&lr=" + pair.Region;
                    }
                    plan.push_back(pair);
                    ++current;
                }
            }
        }
    }

    THashMap<TString, TString> downloadResults;
    while (!downloads.empty()) {
        Cerr << "============================= TRYING REMAINED DOWNLOADS (" << downloads.size() << ") =====================" << Endl;
        TVector<TDownload> retryDownloads;

        THolder<IThreadPool> queue = CreateThreadPool(threadsCount);
        for (size_t threadNumber = 0; threadNumber < threadsCount; ++threadNumber) {
            queue->SafeAddFunc([&, threadNumber] {
                TVector<TDownload> localRetryDownloads;

                for (size_t i = threadNumber; i < downloads.size(); i += threadsCount) {
                    if (!DownloadAndDump(downloads[i], timeout, downloadResults)) {
                        localRetryDownloads.push_back(downloads[i]);
                    }
                }

                if (!localRetryDownloads.empty()) {
                    static TMutex lock;
                    TGuard<TMutex> guard(lock);

                    retryDownloads.insert(retryDownloads.end(), localRetryDownloads.begin(), localRetryDownloads.end());
                }
            });
        }
        queue->Stop();

        std::swap(downloads, retryDownloads);
    }

    TFixedBufferFileOutput outPlan("plan.tsv");
    TFixedBufferFileOutput outCsv("plan.csv");
    for (size_t current = 0; current < plan.size(); ++current) {
        TPlannedPair pair = plan[current];
        if (action == "download" && downloadResults[pair.Left] != downloadResults[pair.Right])
            outPlan << "test" << current << "\t" << pair.Region << "\t" << prefix << pair.Left << "\t" << prefix << pair.Right << Endl;
        else if (action == "plan")
            outPlan << "test" << current << "\t" << pair.Region << "\t" << pair.Left << "\t" << pair.Right << Endl;
        outCsv << WideToChar(UTF8ToWide(pair.Query), CODES_YANDEX) << ";" << pair.Left << ";" << pair.Right;
        for (size_t k = 0; k < betas.size(); ++k) {
            if (k == static_cast<size_t>(pair.LeftN))
                outCsv << ";1";
            else if (k == static_cast<size_t>(pair.RightN))
                outCsv << ";-1";
            else
                outCsv << ";0";
        }
        outCsv << Endl;
    }
}
////////////////////////////////////////////////////////////////////////////////
void SelectStratifiedQueries(TServer& server, const TString &tableName, int cross, const TVector<TBeta> &betas, TString action, TVector<TQueryRegion> *res, bool withProductionOnly)
{
    int neededCount;
    if (action == "list")
        neededCount = cross;
    else
        neededCount = GetNeededQueriesAmount(betas, cross, withProductionOnly);
    int countToSelect = neededCount * 2;

    TClient client(server);
    TTable table(client, tableName);
    TTableIterator it = table.Begin();
    THashMap<TString, TStrata> strata;
    int totalSelectedStrataCount = 0;
    int totalCount = 0;
    for (; it.IsValid(); ++it) {
        TString key = it.GetKey().AsString();
        TString value = it.GetValue().AsString();
        if (key == "*") {
            totalCount += FromString<int>(value);
            continue;
        }
        TStrata &dst = strata[key];
        if (it.GetSubKey().AsString() == "*") {
            int recCount = FromString<int>(value);
            dst.Count += recCount;
            totalSelectedStrataCount += recCount;
        } else {
            dst.Reqs.push(TQueryRegion(value), RandomNumber<ui32>());
            if (dst.Reqs.size() > (size_t)countToSelect)
                dst.Reqs.pop();
        }
    }

    // inform user about his stream rate
    if (totalCount > totalSelectedStrataCount) {
        Cerr << "WARNING: not all query stream was sampled!" << Endl;
        Cerr << "All the profit should be multiplied by approx. " << (totalSelectedStrataCount / (float)totalCount) << Endl;
    }

    // we need to select exactly at given rate from every strata, but we also need to select given amount total
    // too bad that's impossible (at least generally speaking)
    // the following strange process is an attempt to fulfill both constraints with minimal error
    countToSelect = neededCount;
    do {
        SelectStratified(strata, countToSelect, totalSelectedStrataCount, res);
        ++countToSelect;
    } while (res->ysize() < neededCount);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void PrintUsageAndExit() {
    Cerr << "Usage: -T <user_sessions table> -R <region> [-A <action> -F <exp.flags> -X <cross> -p <experiment name>]" << Endl;
    Cerr << "   Several user_sessions or sample_by_yuid_1p/user_sessions tables can be specified with multiple -T option" << Endl;
    Cerr << "       -A <action> -- specify what to do. <action> can be one of: list, plan, download." << Endl;
    Cerr << "           list - print stratified queries into stdout (default)" << Endl;
    Cerr << "           plan - print experiment plans: plan.tsv and plan.csv" << Endl;
    Cerr << "           download - print experiment plans AND download all SERPs" << Endl;
    Cerr << "   If action isn't 'list', at least one exp.flag should be specified with -F option" << Endl;
    Cerr << "   If exp.flag starts with 'http://', it's considered a separate beta/search engine" << Endl;
    Cerr << "Options:" << Endl;
    Cerr << "       -P  -- include porn queries (excluded by default)" << Endl;
    Cerr << "       -R  -- include only queries from this region (if domain is not specified, default is 213)" << Endl;
    Cerr << "       -d  -- include only queries from this domain (ru, ua, tr etc., no filter by default)" << Endl;
    Cerr << "       -U  -- include only queries with this UI language (no filter by default)" << Endl;
    Cerr << "       -S  -- include only queries from these strata (several can be specified, all by default)" << Endl;
    Cerr << "       -B  -- beta name and flags ('hamster.yandex.ru/yandsearch?waitall=da&text=' by default)" << Endl;
    Cerr << "       -b  -- compare experiments only with production (specified in -B)" << Endl;
    Cerr << "       -Q  -- queries list file (do not select queries from MR, use text file with one query per line)" << Endl;
    Cerr << "       -a  -- add URLS for web results to query texts (good for 'list' action)" << Endl;
    Cerr << "       -t  -- threads count for download (one thread by default)" << Endl;
    Cerr << "       -o  -- download timeout (10 seconds by default)" << Endl;
    Cerr << "       -x  -- download x times" << Endl;
    Cerr << "       DEF_MR_SERVER can be overridden with -s <MR server>" << Endl;
    exit(-1);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static void LoadAllowedStrata(TVector<TString> *allowedStrata, THashMap<TString, TString> *urlStrata, TString name) {
    size_t eqPos = name.find('=');
    if (eqPos == TString::npos) {
        allowedStrata->push_back(name);
        return;
    }
    TFileInput in(name.substr(eqPos + 1));
    name = name.substr(0, eqPos);
    allowedStrata->push_back(name);
    TString url;
    while (in.ReadLine(url)) {
        (*urlStrata)[PrepareUrl(url)] = name;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, const char* argv[])
{
    NMR::Initialize(argc, argv);

    TString serverName = GetEnv("DEF_MR_SERVER");
    TVector<TString> tables;
    int region = 213;
    TString domain;
    TString uil;
    int cross = 0;
    TVector<TBeta> betas;
    TString prefix, commonPrefix = "https://ang2-static.yandex-team.ru/finder/";
    TString productionName = "http://hamster.yandex.ru/yandsearch?text=";
    bool withProductionOnly = false;
    bool banPorn = true;
    bool addSerps = false;
    TVector<TString> allowedStrata;
    THashMap<TString, TString> urlStrata;
    TString action = "list";
    TString queriesList;
    size_t threadsCount = 1;
    size_t timeout = 10;
    int times = 1;

    OPTION_HANDLING_PROLOG_ANON("T:R:F:X:s:Pp:B:bS:A:D:Q:t:o:aU:d:x:");
    OPTION_HANDLE('T', (tables.push_back(opt.Arg)));
    OPTION_HANDLE('R', (region = atoi(opt.Arg)));
    OPTION_HANDLE('d', (domain = opt.Arg));
    OPTION_HANDLE('U', (uil = opt.Arg));
    OPTION_HANDLE('F', (betas.push_back(TBeta(opt.Arg))));
    OPTION_HANDLE('D', (betas.back().Deletions.push_back(opt.Arg)));
    OPTION_HANDLE('X', (cross = FromString<int>(opt.Arg)));
    OPTION_HANDLE('s', (serverName = opt.Arg));
    OPTION_HANDLE('P', (banPorn = false));
    OPTION_HANDLE('p', (prefix = commonPrefix + opt.Arg));
    OPTION_HANDLE('B', (productionName = opt.Arg));
    OPTION_HANDLE('b', (withProductionOnly = true));
    OPTION_HANDLE('S', (LoadAllowedStrata(&allowedStrata, &urlStrata, opt.Arg)));
    OPTION_HANDLE('A', (action = opt.Arg));
    OPTION_HANDLE('Q', (queriesList = opt.Arg));
    OPTION_HANDLE('t', (threadsCount = FromString<size_t>(opt.Arg)));
    OPTION_HANDLE('o', (timeout = FromString<size_t>(opt.Arg)));
    OPTION_HANDLE('a', (addSerps = true));
    OPTION_HANDLE('x', (times = FromString<int>(opt.Arg)));
    OPTION_HANDLING_EPILOG;

    if (action != "list" && action != "plan" && action != "download")
        PrintUsageAndExit();

    if (action != "list" && (betas.empty() || prefix.empty()))
        PrintUsageAndExit();

    betas.emplace_back(); // production
    for (size_t n = 0; n < betas.size(); ++n)
    if (betas[n].BetaName.empty())
        betas[n].BetaName = productionName;

    TVector<TQueryRegion> queries;
    if (queriesList.empty()) {

        if (tables.empty() || cross == 0)
            PrintUsageAndExit();

        if (!serverName) {
            Cerr << "DEF_MR_SERVER environment variable or -s parameter should be specified" << Endl;
            return -1;
        }

        TServer server(serverName);

        WithUniqBTTable tmp(server, "feature_stratified");
        TMRParams params;
        for (size_t n = 0; n < tables.size(); ++n) {
            params.AddInputTable(tables[n]);
        }
        params.AddOutputTable(tmp.Name(), UM_REPLACE);
        server.Reduce(params, new TSelectFeatureStratified(region, domain, uil, banPorn, addSerps, allowedStrata, urlStrata));
        server.Sort(tmp.Name());

        SelectStratifiedQueries(server, tmp.Name(), cross, betas, action, &queries, withProductionOnly);
    } else  {
        TString q;
        TFileInput queriesIn(queriesList);
        while (queriesIn.ReadLine(q)) {
            queries.push_back(TQueryRegion(q, region));
        }
    }

    DoFinalAction(queries, cross, betas, region, prefix, action, timeout, threadsCount, withProductionOnly, times);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
