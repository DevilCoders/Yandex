#include <mapreduce/lib/all.h>
#include <mapreduce/library/value/cast.h>
#include <mapreduce/library/temptable/temptable.h>

#include <quality/logs/parse_lib/parse_lib.h>
#include <quality/user_sessions/request_aggregate_lib/all.h>
#include <quality/functionality/turbo/user_metrics_lib/common.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/draft/date.h>
#include <util/digest/murmur.h>
#include <util/generic/size_literals.h>
#include <util/random/random.h>
#include <util/string/subst.h>

using namespace NMR;
using namespace NRA;
using namespace NTurbo;

static const TString DEFAULT_TEST_ID = "0";

class TBaseFetcher : public IReduce {
protected:
    virtual void ProcessRequest(const TRequest& request, TUpdate& output) = 0;
    TBlockStatInfo BlockStat = TBlockStatInfo();
public:
    TBaseFetcher()
    {}
    virtual ~TBaseFetcher() {
    }

    void Do(TValue, TTableIterator & it, TUpdate & output) override {
        TLogsParser prs(new TDefaultLogsParserErrorHandler, BlockStat);

        for (; it.IsValid(); ++it) {
            if (prs.IsFatUser()) {
                return;
            }
            try {
                prs.AddRec(it.GetKey(), it.GetSubKey(), it.GetValue());
            } catch (...) {
                return;
            }
        }

        try {
            prs.Join();
        } catch (...) {
            return;
        }

        TRequestsContainer cont(prs.GetRequestsContainer());

        for (TRequestIterator<TRequest> iter(cont); iter.Valid(); ++iter) {
            ProcessRequest(*iter, output);
        }
    }
};

struct TReadMoreStats {
    ui64 ReadMoreClicks = 0;
    ui64 ReadMoreShows = 0;
    ui64 GeneralizedReadMoreShows = 0;
    ui64 Urls = 0;
    ui64 ReadMoreClicksOnSerpOnly = 0;
    ui64 ReadMoreClicksOnSerp = 0;
    ui64 Serps = 0;
    ui64 ClicksOnSerp = 0;
    ui64 SerpsWithTurbo = 0;
    ui64 ClicksOnTurbo = 0;
    ui64 ClicksOnSerpWithTurbo = 0;
    ui64 SerpsWithClickedTurbo = 0;
};

class TFetcher : public TBaseFetcher {
private :
    OBJECT_METHODS(TFetcher);
    TVector<TString> TestIds;
    THashMap<TString, TReadMoreStats> StatsByTestid;

    SAVELOAD_OVERRIDE_WITHOUT_BASE(TestIds, BlockStat);

    TMaybe<TString> GetTestId(const TRequest& request) {
        if (TestIds.empty()) {
            return DEFAULT_TEST_ID;
        }

        const auto testProps = dynamic_cast<const NRA::TTestRequestProperties*>(&request);
        for (const auto& testId : TestIds)
            if (testProps && testProps->HasTestID(testId))
                return testId;
        return Nothing();
    }


    void ProcessRequest(const TRequest& request, TUpdate& /*output*/) override {
        const TTouchYandexWebRequest* touchRequest = dynamic_cast<const TTouchYandexWebRequest*>(&request);
        if (!touchRequest) {
            return;
        }

        TMaybe<TString> curTestId = GetTestId(request);
        if (!curTestId) {
            return;
        }
        TReadMoreStats& dst = StatsByTestid[*curTestId];

        bool hasBNA = false;
        const TMiscRequestProperties* mrp = dynamic_cast<const TMiscRequestProperties*>(&request);
        if (mrp != nullptr) {
            const NRA::TNameValueMap& sprops = mrp->GetSearchPropsValues();
            auto it1 = sprops.find("WEB.BigNavAnswer.should_show");
            auto it2 = sprops.find("UPPER.BigNavAnswerUpper.in_white_list");
            bool shouldShow = ((it1 != sprops.end() && it1->second == "1") ||
                               (it2 != sprops.end() && it2->second == "1"));
            if (shouldShow) {
                auto it = sprops.find("UPPER.BigNavAnswerUpper.urls_nm");
                if (it != sprops.end() && it->second != "0") {
                     hasBNA = true;
                }
            }
        }
        if (hasBNA) {
            ++dst.GeneralizedReadMoreShows;
        }

        const TBlockstatRequestProperties* bsProps = dynamic_cast<const TBlockstatRequestProperties*>(&request);
        if (!bsProps) {
            return;
        }
        for (const auto& block : bsProps->GetBSBlocks()) {
            TString path = block->GetPath();
            if (!path.Contains("web/item/more")) {
                continue;
            }
            ++dst.ReadMoreShows;
            if (hasBNA) {
                bool isFirstResult = false;
                for (const auto& var : block->GetVars()) {
                    if (var.first == "pos" && var.second == "p0") {
                        isFirstResult = true;
                    }
                }
                if (isFirstResult) {
                    continue;
                }
            }
            ++dst.GeneralizedReadMoreShows;
        }
        bool hasTurbo = false;
        const NRA::TBlocks& blocks = request.GetMainBlocks();
        for (size_t pos = 0; pos < blocks.size(); ++pos) {
            const NRA::TResult* mainResult = blocks[pos]->GetMainResult();
            const NRA::TWebResult* webResult = dynamic_cast<const NRA::TWebResult*>(mainResult);
            if (webResult) {
                ++dst.Urls;
                if (IsTurboResult(webResult)) {
                    hasTurbo = true;
                }
            }
        }
        dst.SerpsWithTurbo += hasTurbo;
        bool hasReadMoreClicks = false;
        bool hasOtherClicks = false;
        bool hasTurboClicks = false;
        const auto& clicks = request.GetClicks();
        for (const auto& click : clicks) {
            if (IsTurboClick(click.Get())) {
                ++dst.ClicksOnTurbo;
                hasTurboClicks = true;
            }
            if (!click->IsDynamic()) {
                ++dst.ClicksOnSerp;
                dst.ClicksOnSerpWithTurbo += hasTurbo;
            }
            TString path = click->GetPath();
            if (path != "80.22.75") {
                if (path.StartsWith("80.22.") && path != "80.22.486") {
                    hasOtherClicks = true;
                }
                continue;
            }
            hasReadMoreClicks = true;
            ++dst.ReadMoreClicks;
        }
        if (hasReadMoreClicks) {
            if (!hasOtherClicks) {
                ++dst.ReadMoreClicksOnSerpOnly;
            }
            ++dst.ReadMoreClicksOnSerp;
        }
        dst.SerpsWithClickedTurbo += hasTurboClicks;
        ++dst.Serps;
    }

    TFetcher() {
    };

public:
    TFetcher(const TVector<TString>& testIds, const TBlockStatInfo& blockstat)
        : TestIds(testIds)
    {
        BlockStat = blockstat;
    }

    virtual ~TFetcher() {
    }
    void Finish(ui32, TUpdate& output) override {
        for (const auto& it : StatsByTestid) {
            TString testId = it.first;
            const TReadMoreStats& stats = it.second;
            output.Add(testId,  ToString(stats.ReadMoreClicks) + " " + ToString(stats.ReadMoreShows) + " " +
                    ToString(stats.GeneralizedReadMoreShows) + " " + ToString(stats.Urls) + " " +
                    ToString(stats.ReadMoreClicksOnSerpOnly) + " " + ToString(stats.ReadMoreClicksOnSerp) + " " +
                    ToString(stats.Serps) + " " + ToString(stats.ClicksOnSerp) + " " +
                    ToString(stats.SerpsWithTurbo) + " " + ToString(stats.ClicksOnTurbo) + " " +
                    ToString(stats.ClicksOnSerpWithTurbo) + " " + ToString(stats.SerpsWithClickedTurbo));
        }
    }
};

REGISTER_SAVELOAD_CLASS(0x1234, TFetcher);

class TOptions {
public:
    TDate MinDate;
    TDate MaxDate;
    TString Timestamp;
    TString TestIdsStr;
    TString OutputPrefix;
    TString Blockstat;
    TVector<TString> TestIds;
    bool SampleByUid = false;
public:
    TOptions(int argc, const char* argv[]) {
        NLastGetopt::TOpts opts;
        opts.AddHelpOption();
        opts.AddCharOption('m', "min date of period (e.g. 20160101)")
            .RequiredArgument("DATE")
            .StoreResult(&MinDate);
        opts.AddCharOption('M', "max date of period (e.g. 20160101)")
            .RequiredArgument("DATE")
            .StoreResult(&MaxDate);
        opts.AddCharOption('T', "timestamp")
            .RequiredArgument("TIMESTAMP")
            .StoreResult(&Timestamp);
        opts.AddCharOption('t', "optional - coma-separated list of testids \
                to calc read more coverage in each of them")
            .StoreResult(&TestIdsStr);
        opts.AddCharOption('o', "output prefix for mr tables")
            .StoreResult(&OutputPrefix)
            .DefaultValue("home/search-functionality/steiner");
        opts.AddLongOption("fast", "sample by uid (1%)")
            .NoArgument()
            .SetFlag(&SampleByUid);
        opts.AddLongOption("blockstat", "path to blockstat.dict file")
            .StoreResult(&Blockstat)
            .Optional()
            .DefaultValue("blockstat.dict");
        opts.SetFreeArgsMax(0);
        opts.SetAllowSingleDashForLong(true);
        NLastGetopt::TOptsParseResult(&opts, argc, argv);
        if (!TestIdsStr.empty()) {
            Split(TestIdsStr, ",", TestIds);
        }
    }
};

template <class T>
static void DumpVal(const TString& date, const TString& testId, const TString& valueName, T value) {
    Cout << date;
    if (testId != DEFAULT_TEST_ID) {
        Cout << "\t" << testId;
    }
    Cout << "\t" << valueName << "\t" << value << Endl;
}

struct TTableData {
    TString InputTable;
    TString OutputTable;
    TString Name;
};

int main(int argc, const char* argv[]) {
    Initialize(argc, argv);
    TOptions options(argc, argv);
    const TBlockStatInfo blockstat(options.Blockstat);
    TServer server("hahn.yt.yandex.net");
    server.SetDefaultUser("sitelinks");
    TClient client(server);
    TVector<TTableData> tables;
    int rand = RandomNumber<ui32>(1000000);
    if (options.Timestamp.size()) {
             TTableData table;
             table.InputTable = "user_sessions/pub/search/fast/" + options.Timestamp + "/clean";
             table.OutputTable = options.OutputPrefix + "/read_more_" + options.Timestamp + "_" + ToString<int>(rand);
             table.Name = options.Timestamp;
             tables.push_back(table);
    } else {
        TString inputPrefix = (options.SampleByUid ? "user_sessions/pub/sample_by_uid_1p/search/daily/" : "user_sessions/pub/search/daily/");
        for (TDate date = options.MinDate; date <= options.MaxDate; ++date) {
             TTableData table;
             table.InputTable = inputPrefix + date.ToStroka("%Y-%m-%d") + "/clean";
             table.OutputTable = options.OutputPrefix + "/read_more_" + date.ToStroka() + "_" + ToString<int>(rand);
             table.Name = date.ToStroka();
             tables.push_back(table);
        }
    }
    for (size_t i = 0; i < tables.size(); ++i) {
        TMRParams params;
        TString inputTable = tables[i].InputTable;
        params.AddInputTable(inputTable);
        TString outputTable = tables[i].OutputTable;
        params.AddOutputTable(outputTable, UM_REPLACE);
        params.SetJobMemoryLimit(2_GB);
        server.Reduce(params, new TFetcher(options.TestIds, blockstat));
        TString outputTableSorted = outputTable + "_sorted";
        server.Sort(outputTable, outputTableSorted);
        server.Drop(outputTable);
        TTable table(client, outputTableSorted);
        THashMap<TString, TReadMoreStats> readMoreStatsByTestId;
        for (TTableIterator it = table.Begin(); it.IsValid(); ++it) {
            if (!it.GetValue().AsString().Contains(" ")) {
                Cout << it.GetValue().AsString() << Endl;
                continue;
            }
            TReadMoreStats tmpStats;
            Split(it.GetValue().AsString(), ' ',
                  tmpStats.ReadMoreClicks,
                  tmpStats.ReadMoreShows,
                  tmpStats.GeneralizedReadMoreShows,
                  tmpStats.Urls,
                  tmpStats.ReadMoreClicksOnSerpOnly,
                  tmpStats.ReadMoreClicksOnSerp,
                  tmpStats.Serps,
                  tmpStats.ClicksOnSerp,
                  tmpStats.SerpsWithTurbo,
                  tmpStats.ClicksOnTurbo,
                  tmpStats.ClicksOnSerpWithTurbo,
                  tmpStats.SerpsWithClickedTurbo
            );
            TReadMoreStats& dst = readMoreStatsByTestId[it.GetKey().AsString()];
            dst.ReadMoreClicks += tmpStats.ReadMoreClicks;
            dst.ReadMoreShows += tmpStats.ReadMoreShows;
            dst.GeneralizedReadMoreShows += tmpStats.GeneralizedReadMoreShows;
            dst.Urls += tmpStats.Urls;
            dst.ReadMoreClicksOnSerpOnly += tmpStats.ReadMoreClicksOnSerpOnly;
            dst.ReadMoreClicksOnSerp += tmpStats.ReadMoreClicksOnSerp;
            dst.Serps += tmpStats.Serps;
            dst.ClicksOnSerp += tmpStats.ClicksOnSerp;
            dst.SerpsWithTurbo += tmpStats.SerpsWithTurbo;
            dst.ClicksOnTurbo += tmpStats.ClicksOnTurbo;
            dst.ClicksOnSerpWithTurbo += tmpStats.ClicksOnSerpWithTurbo;
            dst.SerpsWithClickedTurbo += tmpStats.SerpsWithClickedTurbo;
        }
        for (THashMap<TString, TReadMoreStats>::iterator it = readMoreStatsByTestId.begin(); it != readMoreStatsByTestId.end(); ++it) {
            TString testId = it->first;
            TReadMoreStats& rmStats = it->second;

            DumpVal(tables[i].Name, testId, "read_more_touch_percentage",
                    100.0 * rmStats.ReadMoreShows / (rmStats.Urls > 0 ? rmStats.Urls : 1));
            DumpVal(tables[i].Name, testId, "generalized_read_more_touch_percentage",
                    100.0 * rmStats.GeneralizedReadMoreShows / (rmStats.Urls > 0 ? rmStats.Urls : 1));
            DumpVal(tables[i].Name, testId, "read_more_touch_ctr",
                    100.0 * rmStats.ReadMoreClicks / (rmStats.ReadMoreShows > 0 ? rmStats.ReadMoreShows : 1));
            DumpVal(tables[i].Name, testId, "read_more_touch_clicks_only",
                    100.0 * rmStats.ReadMoreClicksOnSerpOnly / (rmStats.Serps > 0 ? rmStats.Serps : 1));
            DumpVal(tables[i].Name, testId, "read_more_touch_serp_ctr",
                    100.0 * rmStats.ReadMoreClicksOnSerp / (rmStats.Serps > 0 ? rmStats.Serps : 1));

            DumpVal(tables[i].Name, testId, "touch|serps|count", rmStats.Serps);
            DumpVal(tables[i].Name, testId, "touch|serps|clicks|count", rmStats.ClicksOnSerp);
            DumpVal(tables[i].Name, testId, "touch|serps_with_turbo|count", rmStats.SerpsWithTurbo);
            DumpVal(tables[i].Name, testId, "touch|serps_with_turbo|clicks|all|count", rmStats.ClicksOnSerpWithTurbo);
            DumpVal(tables[i].Name, testId, "touch|serps_with_turbo|clicks|turbo|count", rmStats.ClicksOnTurbo);
            DumpVal(tables[i].Name, testId, "touch|serps_with_clicked_turbo|count", rmStats.SerpsWithClickedTurbo);

            DumpVal(tables[i].Name, testId, "touch|serps_with_turbo|percent",
                    rmStats.Serps > 0 ? 100.0 * rmStats.SerpsWithTurbo / rmStats.Serps : 0.0);
            DumpVal(tables[i].Name, testId, "touch|serps_with_clicked_turbo|percent",
                    rmStats.Serps > 0 ? 100.0 * rmStats.SerpsWithClickedTurbo / rmStats.Serps : 0.0);
            DumpVal(tables[i].Name, testId, "touch|serps_with_clicked_turbo_vs_serps_with_turbo|percent",
                    rmStats.SerpsWithTurbo > 0 ? 100.0 * rmStats.SerpsWithClickedTurbo / rmStats.SerpsWithTurbo : 0.0);
            DumpVal(tables[i].Name, testId, "touch|turbo_clicks_vs_serps_clicks|percent",
                    rmStats.ClicksOnSerp > 0 ? 100.0 * rmStats.ClicksOnTurbo / rmStats.ClicksOnSerp : 0.0);
            DumpVal(tables[i].Name, testId, "touch|turbo_clicks_vs_serps_with_turbo_clicks|percent",
                    rmStats.ClicksOnSerpWithTurbo > 0 ? 100.0 * rmStats.ClicksOnTurbo / rmStats.ClicksOnSerpWithTurbo : 0.0);
        }
    }
    return 0;
}
