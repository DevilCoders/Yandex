#include "options.h"

#include <mapreduce/lib/all.h>
#include <quality/logs/parse_lib/parse_lib.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/unidata.h>
#include <library/cpp/digest/md5/md5.h>
#include <util/draft/date.h>
#include <util/folder/dirut.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/random/random.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/fs.h>

using namespace NMR;

class TUserSession2SnippetTypes : public IMap {
    OBJECT_METHODS(TUserSession2SnippetTypes);

private:
    TStraightForwardParsingRules ParsingRules;
    TString Key;

private:
    int operator&(IBinSaver& f) override {
        f.Add(0, &Key);
        return 0;
    }

    TUserSession2SnippetTypes() {
    }

public:
    TUserSession2SnippetTypes(const TString& key)
        : Key(key)
    {
    }

    void DoSub(TValue key, TValue subKey, TValue value, TUpdate& output) override {
        try {
            const TActionItem* item = ParsingRules.ParseMRData(key, subKey, value);
            if (!item || !item->IsA(AT_YANDEX_WEB_SEARCH_REQUEST)) {
                return;
            }
            const TYandexWebSearchRequestItem* requestItem =
                static_cast<const TYandexWebSearchRequestItem*>(item);

            auto& propsValues = requestItem->GetSearchPropsValues();
            bool webHasBNA = propsValues.find("WEB.BigNavAnswer.work") != propsValues.end();
            TVector<TString> res;
            if (webHasBNA)
                res.push_back("__bna");
            TDocReportItems answers;
            requestItem->FillAnswers(&answers);
            for (auto&& answer : answers)
                res.push_back(answer.SnippetType);
            output.AddSub(Key, requestItem->GetServiceDomRegion(), JoinSeq(",", res));
        } catch (...) {
            Cerr << CurrentExceptionMessage() << Endl;
        }
    }
};
REGISTER_SAVELOAD_CLASS(0xAAAA0003, TUserSession2SnippetTypes);

using TSnippetTypesCount = THashMap<TString, size_t>;
using TDomainSnippetTypes = THashMap<TString, TSnippetTypesCount>;
class TSnippetsTypes2SnippetTypesCount: public IReduce {
    OBJECT_METHODS(TSnippetsTypes2SnippetTypesCount);
    mutable TDomainSnippetTypes DomainSnippetTypes;
    mutable TString Key;
public:
    TSnippetsTypes2SnippetTypesCount(){}

    void Do(TValue key, TTableIterator &input, TUpdate&) override {
        Key = key.AsString();
        TVector<TStringBuf> snippetTypes;
        TContainerConsumer<TVector<TStringBuf>> consumer(&snippetTypes);
        TCharDelimiter<const char> delim(',');
        for (; input.IsValid(); ++input) {
            snippetTypes.clear();
            TString domain = input.GetSubKey().AsString();
            TSnippetTypesCount& allSnippetTypes = DomainSnippetTypes[domain];
            TString value = input.GetValue().AsString();
            SplitString(value.data(), delim, consumer);
            for (const auto& snippetType : snippetTypes) {
                auto it = allSnippetTypes.find(snippetType);
                if (it == allSnippetTypes.end())
                    allSnippetTypes[snippetType] = 1;
                else
                    ++(it->second);
            }
        }
    }

    void Finish(ui32, TUpdate& output) override {
        for (const auto& pair : DomainSnippetTypes) {
            const TString& domain = pair.first;
            const TSnippetTypesCount& allSnippetTypes = pair.second;
            TStringBuilder builder;
            for (const auto& it : allSnippetTypes) {
               if (!builder.Empty())
                   builder << ",";
               builder << ToString(it.first) << ";" << it.second;
            }
            output.AddSub(Key, domain, builder);
        }
        Key.clear();
        DomainSnippetTypes.clear();
   }

};
REGISTER_SAVELOAD_CLASS(0xAAAA0004, TSnippetsTypes2SnippetTypesCount);

namespace {

static void TableToFile(TServer& server, const TString& tableName, const TOptions& options) {
    TClient client(server);
    TTable table(client, tableName);
    Cerr << "Total number of records in " << tableName << ": " << table.GetRecordCount() << Endl;
    TFixedBufferFileOutput file(options.StatisticsFileName);
    size_t outputLines = 0;
    TString prevKey;
    for (TTableIterator data = table.Begin(); data.IsValid(); ++data) {
        file << data.GetKey().AsString() << "\t" <<
                data.GetSubKey().AsString() << "\t" <<
                data.GetValue().AsString() << "\n";
        ++outputLines;
    }
    Cerr << "Saved " << outputLines << " lines into file " << options.StatisticsFileName << Endl;
}

static const char* const SAMPLE_BY_UID = "sample_by_yuid_1p/user_sessions/";
static const char* const USER_SESSIONS = "user_sessions/";
static const char* const RESULT_TABLE_DIR = "snippets_kpi/";
static const char* const SNIPPET_TYPES = "snippets_kpi/snippet_types/";
static const char* const SNIPPET_TYPES_COUNT = "snippets_kpi/snippet_types_count/";

static bool ExistsTable(TServer& server, const TString& table) {
    TVector<NMR::TTableInfo> attrs;
    server.GetTables(&attrs, NMR::EGetTablesType::GT_EXACT_MATCH, table.c_str());
    return attrs.size() > 0;
}

static void ExecuteMR(const TOptions& options) {
    try {
        TServer server(options.ServerName);
        server.SetDefaultUser(options.UserName);
        TString tablesPrefix = options.SampleByUid ? SAMPLE_BY_UID : USER_SESSIONS;
        TString fastPrefix = options.SampleByUid ? "1p/" : "";
        const TString statTable = TStringBuilder() << RESULT_TABLE_DIR <<
                         "res_" << options.MinDate << "_" << options.MaxDate <<
                         "_" << RandomNumber<ui32>(1000) << "_reduce";
        TVector<TString> dayTables;
        for (TDate date = options.MinDate; date <= options.MaxDate; ++date) {
            TString dateSuffix = date.ToStroka();
            TString inputTable = tablesPrefix + dateSuffix;
            TString outputTable = SNIPPET_TYPES + fastPrefix + dateSuffix;
            TString dayReduceTable = SNIPPET_TYPES_COUNT + fastPrefix + dateSuffix;
            if (ExistsTable(server, dayReduceTable))
                Cerr << "Skipping " << dayReduceTable << Endl;
            else {
                if (ExistsTable(server, outputTable))
                    Cerr << "Skipping " << outputTable << Endl;
                else {
                    Cerr << "Building " << outputTable << Endl;
                    server.Map(inputTable, outputTable,
                        new TUserSession2SnippetTypes(dateSuffix),
                        UM_REPLACE);
                    Cerr << "Sorting " << outputTable << Endl;
                    server.Sort(outputTable);
                }
                Cerr << "Building " << dayReduceTable << Endl;
                TMRParams params;
                params.EnableReduceWithoutSort();
                params.AddInputTable(outputTable);
                params.AddOutputTable(dayReduceTable, UM_REPLACE);
                server.Reduce(params, new TSnippetsTypes2SnippetTypesCount());
                Cerr << "Sorting " << dayReduceTable << Endl;
                server.Sort(dayReduceTable);
            }
            dayTables.push_back(dayReduceTable);
        }
        Cerr << "Merging to table " << statTable << Endl;
        server.Copy(dayTables, statTable);
        Cerr << "Sorting " << statTable << Endl;
        server.Sort(statTable);
        Cerr << "Fetching results" << Endl;
        TableToFile(server, statTable, options);
        Cerr << "Dropping table " << statTable << Endl;
        server.Drop(statTable);
        Cerr << "Done" << Endl;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
}

static void Execute(TOptions& options) {
    try {
        TString oldStatisticsFileName = options.StatisticsFileName;
        if (!options.StatisticsFileName || !NFs::Exists(options.StatisticsFileName)) {
            if (!options.StatisticsFileName)
                options.StatisticsFileName = TString("tmp_") + ToString(RandomNumber<ui32>(1000));
            ExecuteMR(options);
        }
        TFileInput statFile(options.StatisticsFileName);
        TFixedBufferFileOutput output(options.OutputFileName);
        TString line;
        TVector<TStringBuf> snippetTypes;
        TContainerConsumer<TVector<TStringBuf>> consumer(&snippetTypes);
        TCharDelimiter<const char> delim(',');
        TSet<TString> unknown;
        while (statFile.ReadLine(line)) {
            TString date;
            TString domain;
            TString data;
            Split(line, '\t', date, domain, data);
            if (options.DomainFilter && domain != options.DomainFilter)
                continue;
            snippetTypes.clear();
            SplitString(data.data(), delim, consumer);
            double sum = 0.0;
            size_t countTotal = 0;
            for (const TStringBuf& sbuf : snippetTypes) {
                TString snippetType;
                size_t count;
                Split(sbuf, ';',  snippetType, count);
                if (options.IsUnknownSnippetType(snippetType))
                    unknown.insert(snippetType);
                sum += options.GetK(snippetType) * count;
                countTotal += count;
            }
            output << date << "\t" <<
                    countTotal << "\t" <<
                    sum << "\t" <<
                    ((countTotal == 0) ? 0 : sum / countTotal) << Endl;
        }
        if (!unknown.empty())
            Cerr << "Unknown snippet types: " << JoinSeq(",", unknown) << Endl;
        if (oldStatisticsFileName != options.StatisticsFileName)
            NFs::Remove(options.StatisticsFileName);
        Cerr << "Done" << Endl;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
}

};

int main(int argc, const char* argv[]) {
    Initialize(argc, argv);
    TOptions opt(argc, argv);
    Execute(opt);
    return 0;
}
