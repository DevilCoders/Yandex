#include <tools/snipmake/download_contexts/common/querylog.h>

#include <mapreduce/interface/all.h>
#include <mapreduce/lib/init.h>

#include <quality/logs/parse_lib/parse_lib.h>

#include <kernel/urlid/url2docid.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/digest/md5/md5.h>

#include <util/charset/unidata.h>
#include <util/draft/date.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/random/random.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <library/cpp/string_utils/url/url.h>

using namespace NMR;

static ui64 UrlHash(TStringBuf url)
{
    url.SkipPrefix("http://");
    TString urlLower = NUrl2DocId::GetUrlWithLowerCaseHost(url);
    return ComputeHash(urlLower);
}

static bool TryFindSnipDebugField(const TDocReportItems& answers, const TStringBuf& fieldName, TStringBuf& result) {
    for (const TDocReportItem& answer : answers) {
        auto snipDebug = answer.Markers.find("SnipDebug");
        if (snipDebug != answer.Markers.end()) {
            TStringBuf str(snipDebug->second);
            while (TStringBuf field = str.NextTok(';')) {
                TStringBuf name = field.NextTok('=');
                TStringBuf value = field;
                if (name == fieldName) {
                    result = value;
                    return true;
                }
            }
            break;
        }
    }
    result = TStringBuf();
    return false;
}

static TStringBuf FindSnipDebugField(const TDocReportItems& answers, const TStringBuf& fieldName) {
    TStringBuf result;
    TryFindSnipDebugField(answers, fieldName, result);
    return result;
}

static bool HasSnipDebugField(const TDocReportItems& answers, const TStringBuf& fieldName) {
    TStringBuf unusedResult;
    return TryFindSnipDebugField(answers, fieldName, unusedResult);
}

static TString FormatLog(const TBaseYandexWebSearchRequestItem& request,
        const TDocReportItems& answers, const TString& mrdata) {
    using namespace NSnippets;
    TQueryLog res;
    res.Query = request.GetCorrectedQuery();
    res.FullRequest = request.GetFullRequest();
    TMisspellInfo msp = request.GetMSP();
    if (msp.Misspell && msp.CorrectedQuery) {
        res.CorrectedQuery = TMaybe<TString>(TString(msp.CorrectedQuery));
    }
    res.UserRegion = ToString(request.GetUserRegion());
    res.DomRegion = request.GetServiceDomRegion();
    res.UILanguage = request.GetUILanguage();
    res.RequestId = request.GetReqID();
    TStringBuf snipWidth = FindSnipDebugField(answers, "snip_width");
    if (snipWidth) {
        res.SnipWidth = TMaybe<TString>(TString(snipWidth));
    }
    TStringBuf report = FindSnipDebugField(answers, "report");
    if (report) {
        res.ReportType = TMaybe<TString>(TString(report));
    }
    for (auto&& answer : answers) {
        res.Docs.push_back(TQueryResultDoc(answer.Source, answer.Url, answer.SnippetType));
    }
    if (mrdata) {
        res.MRData = TMaybe<TString>(mrdata);
    }
    return res.ToString();
}

class TSessionToRequestMap : public IMap {
    OBJECT_METHODS(TSessionToRequestMap);

private:
    TStraightForwardParsingRules ParsingRules;
    TString DomainFilter;
    TString UiType;
    TString HostFilter;
    TString SnippetTypeFilter;
    int MaxSnippetPosition;
    bool SampleByRequest = false;
    bool OutputMRData = false;
    bool UniqueRequests = false;
    bool FilterByRegphrase = false;
    THashSet<ui64> UrlIds;
    THashSet<ui64>& SourceUrlIds;
    bool KeepFullSerp = false;

private:
    int operator&(IBinSaver& f) override {
        f.Add(0, &DomainFilter);
        f.Add(0, &UiType);
        f.Add(0, &HostFilter);
        f.Add(0, &SnippetTypeFilter);
        f.Add(0, &MaxSnippetPosition);
        f.Add(0, &SampleByRequest);
        f.Add(0, &OutputMRData);
        f.Add(0, &UniqueRequests);
        f.Add(0, &FilterByRegphrase);
        f.Add(0, &SourceUrlIds);
        f.Add(0, &KeepFullSerp);
        return 0;
    }

    TSessionToRequestMap()
        : SourceUrlIds(UrlIds)
    {
    }

    TString CalcRequestHash(const TBaseYandexWebSearchRequestItem& request,
            const TDocReportItems& answers) const {
        TStringBuilder sb;
        if (UniqueRequests) {
            sb << "\t" << request.GetCorrectedQuery();
            TMisspellInfo msp = request.GetMSP();
            if (msp.Misspell && msp.CorrectedQuery) {
                sb << "\t" << msp.CorrectedQuery;
            }
            sb << "\t" << request.GetServiceDomRegion();
            sb << "\t" << request.GetUILanguage();
            TVector<TString> urls;
            for (auto&& answer : answers) {
                urls.push_back(answer.Url);
            }
            Sort(urls.begin(), urls.end());
            for (auto&& url : urls) {
                sb << "\t" << url;
            }
        } else {
            sb << request.GetReqID();
        }
        return MD5::Calc(sb);
    }

public:
    TSessionToRequestMap(const TString& domainFilter, const TString& hostFilter,
            const TString& snippetTypeFilter, int maxSnippetPosition, bool sampleByRequest, bool outputMRData,
            bool uniqueRequests, bool filterByRegphrase, THashSet<ui64>& urlIds, const TString& uiType, bool keepFullSerp)
        : DomainFilter(domainFilter)
        , UiType(uiType)
        , HostFilter(hostFilter)
        , SnippetTypeFilter(snippetTypeFilter)
        , MaxSnippetPosition(maxSnippetPosition)
        , SampleByRequest(sampleByRequest)
        , OutputMRData(outputMRData)
        , UniqueRequests(uniqueRequests)
        , FilterByRegphrase(filterByRegphrase)
        , SourceUrlIds(urlIds)
        , KeepFullSerp(keepFullSerp)
    {
    }

    void DoSub(TValue key, TValue subKey, TValue value, TUpdate& output) override {
        try {
            if (SampleByRequest && RandomNumber<ui32>(100) != 0) {
                return;
            }
            const TActionItem* item = ParsingRules.ParseMRData(key, subKey, value);
            if (!item || !item->IsA(AT_BASE_YANDEX_WEB_SEARCH_REQUEST)) {
                return;
            }
            if (UiType == "desktop" && !item->IsA(AT_YANDEX_WEB_SEARCH_REQUEST)) {
                return;
            }
            if (UiType == "touch" && !item->IsA(AT_TOUCH_YANDEX_WEB_SEARCH_REQUEST)) {
                return;
            }
            const TBaseYandexWebSearchRequestItem* requestItem =
                static_cast<const TBaseYandexWebSearchRequestItem*>(item);
            if (DomainFilter && requestItem->GetServiceDomRegion() != DomainFilter) {
                return;
            }
            TDocReportItems answers;
            requestItem->FillAnswers(&answers);
            TDocReportItems filteredAnswers;
            for (auto&& answer : answers) {
                if (HostFilter && CutWWWPrefix(GetOnlyHost(answer.Url)) != HostFilter) {
                    continue;
                }
                if (SnippetTypeFilter && answer.SnippetType != SnippetTypeFilter) {
                    continue;
                }
                if (MaxSnippetPosition >= 0 && answer.Num > static_cast<ui32>(MaxSnippetPosition)) {
                    continue;
                }
                if (UrlIds && !UrlIds.contains(UrlHash(answer.Url))) {
                    continue;
                }

                filteredAnswers.push_back(answer);
            }
            if (!filteredAnswers) {
                return;
            }
            if (FilterByRegphrase && !HasSnipDebugField(filteredAnswers, "regphrase")) {
                return;
            }
            if (KeepFullSerp) {
                filteredAnswers = answers;
            }
            TString hashKey = CalcRequestHash(*requestItem, filteredAnswers);
            TString mrdata = OutputMRData ? value.AsString() : "";
            TString res = FormatLog(*requestItem, filteredAnswers, mrdata);
            output.Add(hashKey, res);
        } catch (...) {
            Cerr << CurrentExceptionMessage() << Endl;
        }
    }
};
REGISTER_SAVELOAD_CLASS(0xAAAA0000, TSessionToRequestMap);

class TOptions {
public:
    TString ServerName;
    TDate MinDate;
    TDate MaxDate;
    TString DomainFilter;
    TString UiType;
    TString HostFilter;
    TString UrlListFile;
    TString SnippetTypeFilter;
    int MaxSnippetPosition = -1;
    size_t RequestsCount = 0;
    TString OutputFileName;
    bool SampleByUid = false;
    bool SampleByRequest = false;
    bool OutputMRData = false;
    bool UniqueRequests = false;
    bool FilterByRegphrase = false;
    bool KeepFullSerp = false;
public:
    TOptions(int argc, const char* argv[]);
};

void TableToFile(TServer& server, const TString& tableName, const TOptions& options) {
    TClient client(server);
    TTable table(client, tableName);
    Cout << "Total number of records in " << tableName << ": " << table.GetRecordCount() << Endl;
    TFixedBufferFileOutput file(options.OutputFileName);
    size_t outputLines = 0;
    TString prevKey;
    for (TTableIterator data = table.Begin(); data.IsValid(); ++data) {
        TString key = data.GetKey().AsString();
        if (key == prevKey) {
            continue;
        }
        prevKey = key;
        file << data.GetValue().AsString() << "\n";
        ++outputLines;
        if (options.RequestsCount && outputLines >= options.RequestsCount) {
            break;
        }
    }
    Cout << "Saved " << outputLines << " requests into file " << options.OutputFileName << Endl;
}

static const char* const SAMPLE_BY_UID = "user_sessions/pub/sample_by_uid_1p/search/daily/";
static const char* const USER_SESSIONS = "user_sessions/pub/search/daily/";
static const char* const RESULT_TABLE_DIR = "tmp/snippets_contexts/";

static void LoadUrlList(const TString& fileName, THashSet<ui64>& result)
{
    TFileInput inf(fileName);
    TString line;
    while (inf.ReadLine(line)) {
        TStringBuf lineBuf(line);
        lineBuf = StripString(lineBuf);
        if (!lineBuf) {
            continue;
        }

        result.insert(UrlHash(lineBuf));
    }
}

static void Execute(const TOptions& options) {
    try {
        THashSet<ui64> UrlIds;

        if (options.UrlListFile) {
            Cout << "Loading URL list";
            LoadUrlList(options.UrlListFile, UrlIds);
        }

        Cout << "collect_qurls" << Endl;
        TServer server(options.ServerName);
        const TString outputTable = TStringBuilder()
            << RESULT_TABLE_DIR << "res_" << options.MinDate << "_"
            << options.MaxDate << "_" << RandomNumber<ui32>(1000);
        Cout << "Results will be temporarily stored in table " << outputTable << Endl;
        TString tablesPrefix = options.SampleByUid ? SAMPLE_BY_UID : USER_SESSIONS;
        server.Drop(outputTable);
        for (TDate date = options.MinDate; date <= options.MaxDate; ++date) {
            TString inputTable = tablesPrefix + date.ToStroka("%Y-%m-%d") + "/clean";
            Cout << "TSessionToRequestMap for " << inputTable << Endl;
            server.Map(inputTable, outputTable,
                new TSessionToRequestMap(options.DomainFilter, ToString(CutWWWPrefix(GetOnlyHost(options.HostFilter))),
                    options.SnippetTypeFilter, options.MaxSnippetPosition, options.SampleByRequest,
                    options.OutputMRData, options.UniqueRequests,
                    options.FilterByRegphrase, UrlIds, options.UiType, options.KeepFullSerp),
                UM_APPEND);
            Cout << "Sorting table" << Endl;
            server.Sort(outputTable);
        }

        Cout << "Fetching results" << Endl;
        TableToFile(server, outputTable, options);

        Cout << "Drop table " << outputTable << Endl;
        server.Drop(outputTable);

        Cout << "Done" << Endl;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
}

TOptions::TOptions(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.AddHelpOption();
    opts.AddCharOption('s', "YT server")
        .DefaultValue("hahn")
        .StoreResult(&ServerName);
    opts.AddCharOption('m', "min date of period (e.g. 20141001)")
        .Required()
        .RequiredArgument("DATE")
        .StoreResult(&MinDate);
    opts.AddCharOption('M', "max date of period (e.g. 20141031)")
        .Required()
        .RequiredArgument("DATE")
        .StoreResult(&MaxDate);
    opts.AddCharOption('d', "filter by domain (e.g. ru, ua, tr)")
        .StoreResult(&DomainFilter, "");
    opts.AddLongOption("ui", "filter by ui type (desktop or touch)")
        .StoreResult(&UiType, "");
    opts.AddCharOption('h', "filter by host")
        .StoreResult(&HostFilter, "");
    opts.AddLongOption("urls", "filter by url (argument: file containing the list of permitted urls, one per line)")
        .RequiredArgument("URL_LIST_FILE").StoreResult(&UrlListFile, "");
    opts.AddCharOption('t', "filter by snippet type (headline_src)")
        .StoreResult(&SnippetTypeFilter, "");
    opts.AddLongOption("regphrase", "filter requests with regphrase")
        .NoArgument()
        .SetFlag(&FilterByRegphrase);
    opts.AddCharOption('p', "max snippet position, default:unlimited")
        .StoreResult(&MaxSnippetPosition, -1);
    opts.AddLongOption("full-serp", "output the full serp if any of the snippets matches the type/host/url filters")
        .SetFlag(&KeepFullSerp);
    opts.AddCharOption('q', "number of requests to collect")
        .StoreResult(&RequestsCount, 100000);
    opts.AddCharOption('r', "file to write result")
        .Required()
        .RequiredArgument("PATH")
        .StoreResult(&OutputFileName);
    opts.AddLongOption("fast", "sample by uid (1%)")
        .NoArgument()
        .SetFlag(&SampleByUid);
    opts.AddLongOption("percent", "sample by request (1%)")
        .NoArgument()
        .SetFlag(&SampleByRequest);
    opts.AddLongOption("mrdata", "output raw MR data after double TAB")
        .NoArgument()
        .SetFlag(&OutputMRData);
    opts.AddLongOption("uniq", "output unique requests by query + domain + uil + urls")
        .NoArgument()
        .SetFlag(&UniqueRequests);
    opts.SetFreeArgsMax(0);
    opts.SetAllowSingleDashForLong(true);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);
}

int main(int argc, const char* argv[]) {
    NMR::Initialize(argc, argv);
    Execute(TOptions(argc, argv));
    return 0;
}
