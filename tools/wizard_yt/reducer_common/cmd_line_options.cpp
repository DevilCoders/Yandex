#include "cmd_line_options.h"
#include <tools/wizard_yt/reducer_common/rule_updater.h>

#include <util/generic/set.h>
#include <util/generic/queue.h>
#include <util/system/env.h>
#include <util/system/fs.h>
#include <util/thread/pool.h>
#include <util/string/strip.h>
#include <util/folder/path.h>

#include <library/cpp/oauth/oauth.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/streams/factory/factory.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/serialize.h>
#include <search/begemot/core/rulefactory.h>
#include <contrib/libs/re2/re2/stringpiece.h>
#include <contrib/libs/re2/re2/re2.h>


TYtTokenCommandLineOption::TYtTokenCommandLineOption(
    NLastGetopt::TOpts& options,
    const TStringBuf& clientId,
    const TStringBuf& clientSecret
)
    : ClientId(clientId)
    , ClientSecret(clientSecret)
{
    options
        .AddLongOption("token", "YT token")
        .Optional()
        .RequiredArgument("TOKEN")
        .StoreResult(&Token);
}

TString TYtTokenCommandLineOption::Get() {
    auto environmentToken = GetEnv("YT_TOKEN", "");
    auto storedToken = GetEnv("HOME") / TFsPath(".yt/token");
    if (!Token.Empty()) {
        return Token;
    } else if (!environmentToken.empty()) {
        return environmentToken;
    } else if (storedToken.Exists()) {
        return Strip(OpenInput(storedToken.GetPath())->ReadAll());
    } else {
        return GetOauthToken({ ClientId, ClientSecret });
    }
}

TYtProxyCommandLineOption::TYtProxyCommandLineOption(NLastGetopt::TOpts& options) {
    options
        .AddLongOption("proxy", "YT proxy")
        .Optional()
        .RequiredArgument("PROXY")
        .StoreResult(&Proxy);
}

TString TYtProxyCommandLineOption::Get() {
    TString environmentProxy = GetEnv("YT_PROXY", "");
    if (!Proxy.Empty()) {
        return Proxy;
    } else {
        Y_VERIFY(!environmentProxy.empty(), "You must specify proxy");
        return environmentProxy;
    }
}

TJobCountConfigCommandLineOptions::TJobCountConfigCommandLineOptions(NLastGetopt::TOpts& options)
    : JobCount(0)
    , RecordPerJobCount(0)
{
    options
        .AddLongOption('J', "job_count", "job count limit")
        .Optional()
        .RequiredArgument("COUNT")
        .StoreResult(&JobCount);

    options
        .AddLongOption("per_job", "Limit count of records per job")
        .Optional()
        .RequiredArgument("COUNT_PER_JOB")
        .StoreResult(&RecordPerJobCount);
}

i64 TJobCountConfigCommandLineOptions::Get(const TString& inputTable, NYT::IClientPtr& client) {
    if (JobCount > 0) {
        return JobCount;
    }
    Y_VERIFY(RecordPerJobCount > 0, "Both options are undefined");
    const auto& rowCountNode = client->Get(inputTable + "/@row_count");
    Y_VERIFY(rowCountNode.IsInt64() || rowCountNode.IsUint64(), "Invalid type of row count node");
    i64 rowCount = rowCountNode.IntCast<i64>();
    return rowCount < RecordPerJobCount ? 1 : rowCount / RecordPerJobCount;
}

TDataFilesCommandLineOptions::TDataFilesCommandLineOptions(
    const TString& name,
    NLastGetopt::TOpts& options
)
    : ExpirationDays(0)
{
    options
        .AddLongOption(name + "_path", "Path to " + name + " cache directory in local FS")
        .RequiredArgument("FS_SHARD")
        .StoreResult(&LocalPath);

    options
        .AddLongOption("cypress_" + name, "Path to " + name + " directory in Cypress")
        .RequiredArgument("SHARD")
        .StoreResult(&CypressPath);

    options
        .AddLongOption("cypress_" + name + "_cache", "Path to " + name + "cache directory in Cypress")
        .RequiredArgument("SHARD")
        .StoreResult(&CypressCachePath);

    options
        .AddLongOption(name + "_ttl", "Cypress " + name + " cache expirations time in days")
        .RequiredArgument("TTL")
        .Optional()
        .StoreResult(&ExpirationDays);

    options
        .AddLongOption("cypress_" + name + "_file", "File with " + name + " cypress paths to rule directories")
        .RequiredArgument("SHARD_PATHS_FILE")
        .StoreResult(&CypressPathsFile);

}

TDataFilesList TDataFilesCommandLineOptions::Get(NYT::IClientPtr& client, const TVector<TString>& selectedRules) {
    TDataFilesList result{};
    TVector<TString> cypressFiles;
    if (LocalPath.Empty()) {
        cypressFiles = GetCypressFiles(client);
    } else {
        cypressFiles = UploadLocalFiles(client, selectedRules);
    }
    for (auto& cypressPath : cypressFiles) {
        auto localPath = CreateGuidAsString();
        result.Files.push_back({cypressPath, localPath});
        result.TotalSize += client->Get(cypressPath + "/@uncompressed_data_size").AsInt64();
    }
    return result;
}

TVector<TString> TDataFilesCommandLineOptions::UploadLocalFiles(NYT::IClientPtr& client, const TVector<TString>& selectedRules) {
    TVector<TString> resultPaths;
    Y_VERIFY(!CypressCachePath.Empty(), "Cypress path must be defined");
    TThreadPool pool(TThreadPool::TParams().SetBlocking(true).SetCatching(false));

    pool.Start(32);

    client->Create(CypressCachePath, NYT::ENodeType::NT_MAP,
        NYT::TCreateOptions().IgnoreExisting(true).Recursive(true));

    TRuleUpdaterContext ctx;
    ctx.CachePath = CypressCachePath;
    ctx.ExpirationTime = Now() + TDuration::Days(ExpirationDays == 0 ?  30 : ExpirationDays);
    TFsPath fsShard(LocalPath);
    if (fsShard.IsFile()) {
        NJson::TJsonValue parsedConfig;
        TFileInput st(fsShard);
        NJson::ReadJsonFastTree(st.ReadAll(), &parsedConfig, true);
        for (auto& item : parsedConfig["resources"].GetArray()) {
            TString ruleName = item["name"].GetString();
            TFsPath ruleDir = TFsPath(NFs::CurrentWorkingDirectory()) / ruleName;
            if (selectedRules.size() == 0 || FindPtr(selectedRules, ruleName)) {
                TString key;
                if (item.Has("torrent")) {
                    key = item["torrent"].GetString();
                } else if (item.Has("resource_id")) {
                    key = "sbr:" + item["resource_id"].GetString();
                }
                if (key) {
                    pool.SafeAddAndOwn(THolder(new TSandboxRuleDirUpdater(ruleDir, key, client, ctx, resultPaths)));
                }
            }
        }
    } else {
        Y_VERIFY(fsShard.IsDirectory(), "Local path must be exists");
        if (selectedRules.empty()) {
            TVector<TFsPath> children;
            fsShard.List(children);
            for (auto& ruleDir: children) {
                pool.SafeAddAndOwn(THolder(new TRuleDirUpdater(ruleDir, client, ctx, resultPaths)));
            }
        } else {
            for (auto& ruleName: selectedRules) {
                auto ruleDir = fsShard / ruleName;
                if (ruleDir.IsDirectory()) {
                    pool.SafeAddAndOwn(THolder(new TRuleDirUpdater(ruleDir, client, ctx, resultPaths)));
                }
            }
        }
    }

    pool.Stop();
    Y_VERIFY(!ctx.Failed, "Failed to update shard");
    return resultPaths;
}

TVector<TString> TDataFilesCommandLineOptions::GetCypressFiles(NYT::IClientPtr& client) {
    TVector<TString> cypressFiles;
    if (!CypressPath.Empty()) {
        for (const auto& file : client->List(CypressPath)) {
            TString path = CypressPath + "/" + file.AsString();
            if (client->Get(path + "/@type").AsString() == "map_node") {
                Y_VERIFY(client->List(path).size() == 1);
                for (auto file : client->List(path)) {
                    cypressFiles.push_back(path + "/" + file.AsString());
                }
            } else {
                cypressFiles.push_back(path);
            }
        }
    }
    if (!CypressPathsFile.Empty()) {
        TFileInput file(CypressPathsFile);
        TString path;
        while (file.ReadLine(path)) {
            cypressFiles.push_back(path);
        }
    }
    return cypressFiles;
}

TDirectModeCommandLineOptions::TDirectModeCommandLineOptions(NLastGetopt::TOpts& options) {
    options
        .AddLongOption("direct", "process table of queries: QUERY_FIELD[;REGION_VALUE|REGION_FIELD]")
        .RequiredArgument("DIRECT_MODE")
        .Optional()
        .StoreResult(&DirectMode);
}

TDirectModeColumns TDirectModeCommandLineOptions::Get(const TString& inputTable, NYT::IClientPtr& client) {
    TString queryColumn;
    TString regionColumn;
    bool regionIsValue = false;

    if (DirectMode.Empty()) {
        return {};
    }

    Y_VERIFY(re2::RE2::FullMatch(DirectMode, "(\\w+)(?:,(\\d+|[a-zA-Z]\\w*))?", &queryColumn, &regionColumn),
        "Provide two columns for direct mode");

    NYT::TTableSchema inputSchema;
    const auto& schemaNode = client->Get(inputTable + "/@schema");
    NYT::Deserialize(inputSchema, schemaNode);

    TMap<TString, NYT::EValueType> foundColumns;
    if (inputSchema.Columns().empty()) {
        auto reader = client->CreateTableReader<NYT::TNode>(inputTable);
        if (reader->IsValid()) {
            auto row = reader->GetRow();
            for (const auto& item: row.AsMap()) {
                foundColumns.emplace(item.first, NodeTypeToValueType(item.second.GetType()));
            }
        }
    } else {
        for(const auto& column: inputSchema.Columns()) {
            foundColumns.emplace(column.Name(), column.Type());
        }
    }

    Cerr << "Checking columns: " << foundColumns.size() << Endl;
    bool queryFound = false;
    bool regionFound = false;
    regionIsValue = regionColumn.Empty() || (regionColumn[0] >= '0' && regionColumn[0] <= '9');
    for(auto& column: foundColumns) {
        auto name = column.first;
        auto type = column.second;
        if (name.empty())
            continue;
        if (name == queryColumn) {
            queryFound = true;
            Cerr << "- '" << name << "' *query" << Endl;
            Y_VERIFY(type == NYT::VT_STRING, "Query column type must be a string");
        } else if (name == regionColumn && !regionIsValue) {
            regionFound = true;
            Cerr << "- '" << name << "' *region" << Endl;
            Y_VERIFY(type == NYT::VT_INT64 || type == NYT::VT_INT32 || type == NYT::VT_UINT64 || type == NYT::VT_INT32,
                     "Region column type must be at least 32-bit integer");
        } else {
            Cerr << "- '" << name << "'" << Endl;
        }
    }

    Y_VERIFY(queryFound, "Query column not found");
    Y_VERIFY(regionFound || regionIsValue, "Region column not found and region no default value speciefied");
    return {queryColumn, regionColumn, regionIsValue};
}

template <>
TInstant FromStringImpl<TInstant, char>(const char* data, size_t size) {
    return TInstant::ParseIso8601({data, size});
}

