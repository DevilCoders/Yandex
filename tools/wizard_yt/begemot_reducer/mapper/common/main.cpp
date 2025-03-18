#include "main.h"
#include "process_row_task.h"
#include <tools/wizard_yt/reducer_common/common.h>
#include <tools/wizard_yt/reducer_common/rule_updater.h>
#include <tools/wizard_yt/reducer_common/cmd_line_options.h>

#include <mapreduce/yt/common/config.h>

#include <search/begemot/server/server.h>
#include <search/begemot/core/rulefactory.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/string/join.h>
#include <util/system/env.h>

using namespace NYT;

class TBegemotMapper : public TBegemotMapperBase {
private:
    TVector<TString> ShardFiles, FreshFiles;
    ui64 ThreadsCount;
    ui64 CacheSize;
    THolder<NBg::TWorker> Worker;
    THolder<TThreadPool> Mtp;
    THolder<TSerialPostProcessQueue> Queue;

    EBegemotMode Mode;
    NJson::TJsonValue BegemotConfig;
    TString ShardName;
    TString QueryColumn;
    TString RegionColumnOrValue;
    bool IsRegionValue;
    TString AppendCgi;

public:
    TBegemotMapper() {
    }

    TBegemotMapper(
        const TVector<TString>& dataFiles,
        const TVector<TString>& freshFiles,
        ui64 threadsCount,
        ui64 cacheSize,
        EBegemotMode mode,
        const TStringBuf& begemotConfig,
        const TStringBuf& shardName,
        const TStringBuf& queryColumn,
        const TStringBuf& regionColumnOrValue,
        bool isRegionValue,
        const TStringBuf& appendCgi
    )
        : ShardFiles(dataFiles)
        , FreshFiles(freshFiles)
        , ThreadsCount(threadsCount)
        , CacheSize(cacheSize)
        , Mode(mode)
        , ShardName(shardName)
        , QueryColumn(queryColumn)
        , RegionColumnOrValue(regionColumnOrValue)
        , IsRegionValue(isRegionValue)
        , AppendCgi(appendCgi)
    {
        if (begemotConfig.Empty()) {
            BegemotConfig = NJson::JSON_UNDEFINED;
        } else {
            BegemotConfig = NJson::TJsonValue();
            NJson::ReadJsonFastTree(begemotConfig, &BegemotConfig, true);
        }
    }

    Y_SAVELOAD_JOB(ShardFiles, FreshFiles, ThreadsCount, CacheSize, Mode, BegemotConfig,
        ShardName, QueryColumn, RegionColumnOrValue, IsRegionValue, AppendCgi);

    void Start(TWriter*) override {
        TString shardPath("shard"), freshPath("fresh");
        ExtractShard(ShardFiles, shardPath);
        ExtractShard(FreshFiles, freshPath);
        NBg::NProto::TConfig config;
        config.SetEventLog("/dev/null");
        config.SetThreads(ThreadsCount);
        config.SetPrintRrrList(true);
        config.SetPrintRuleGraph(true);
        config.SetCacheSize(CacheSize);
        config.SetDataDir(shardPath);
        config.SetFreshDir(freshPath);
        SetEnv("MKL_CBWR", "COMPATIBLE");

        auto fs = NBg::CreateFileSystem(config);
        Worker.Reset(new NBg::TWorker(NBg::DefaultRuleFactory(), *fs, config,
            NBg::CreateEventLog(config.GetEventLog(), 0), NBg::CreateEventLog(""), NBg::CreateEventLog(""), NBg::CreateEventLog("")));

        Mtp.Reset(new TThreadPool(TThreadPool::TParams().SetBlocking(true).SetCatching(false)));
        Queue.Reset(new TSerialPostProcessQueue(Mtp.Get(), true));
        Queue->Start(ThreadsCount, 1024, 1024);
    }

    void Do(TTableReader<TNode>* reader, TTableWriter<TNode>* writer) override {
        Cerr << "Execution started" << Endl;
        for (int i = 1; reader->IsValid(); reader->Next(), i++) {
            Queue->Add(new TProcessRowTask(*Worker.Get(), reader->GetRow(), *writer, Mode,
                BegemotConfig, ShardName, QueryColumn, RegionColumnOrValue, IsRegionValue,
                AppendCgi));
            if (i % 10000 == 0) {
                Cerr << i / 1000 << "k records done" << Endl;
            }
        }
        Cerr << "Done" << Endl;
    }

    void Finish(TWriter* writer) override {
        Queue->Stop();
        TNode statRow = TNode()
            ("result", GetStat());
        writer->AddRow(statRow, 3);
    }

    TString GetStat() {
        TString stat;
        TStringOutput statStream(stat);
        NSrvKernel::THeaders headers;
        Worker->OnCustomAction("stat", TCgiParameters(), statStream, headers);
        return stat;
    }
};

REGISTER_MAPPER(TBegemotMapper);

int main(int argc, const char **argv) {
    NYT::Initialize(argc, argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(ILogger::INFO));

    NLastGetopt::TOpts options;

    TString inputTable;
    options
        .AddLongOption('i', "input", "Path to input log table")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&inputTable);

    TString outputTable;
    options
        .AddLongOption('o', "output", "Path to output table")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&outputTable);

    TString shardName;
    options
        .AddLongOption('s', "shard", "Begemot shard name")
        .RequiredArgument("SHARD")
        .Optional()
        .StoreResult(&shardName);

    TVector<TString> requiredRules;
    options
        .AddLongOption("rule", "Rule, selected for execution")
        .Optional()
        .RequiredArgument("RULE")
        .AppendTo(&requiredRules);

    TDataFilesCommandLineOptions shardPaths("shard", options);
    TDataFilesCommandLineOptions freshPaths("fresh", options);

    TMaybe<i64> rowCountLimit;
    options
        .AddLongOption('l', "row_count_limit", "sets row_count_limit on reduce operation")
        .Optional()
        .RequiredArgument("LIMIT")
        .StoreResultT<i64>(&rowCountLimit);

    ui64 threadsCount;
    options
        .AddLongOption('t', "threads", "threads count")
        .Optional()
        .RequiredArgument("THREADS")
        .DefaultValue(8)
        .StoreResult(&threadsCount);

    ui64 cacheSize;
    options
        .AddLongOption("cache_size", "begemot cache size")
        .Optional()
        .RequiredArgument("CACHE_SIZE")
        .DefaultValue(0)
        .StoreResult(&cacheSize);

    TJobCountConfigCommandLineOptions jobCountConfig(options);

    bool cgiMode;
    options
        .AddLongOption("cgi", "process requests in cgi format")
        .SetFlag(&cgiMode)
        .NoArgument()
        .Optional();

    TString appendCgi;
    options
        .AddLongOption("append_cgi", "append cgi string to every request in direct mode")
        .Optional()
        .OptionalArgument("APPEND_CGI")
        .StoreResult(&appendCgi);

    TDirectModeCommandLineOptions directMode(options);

    ui64 maxFailedJobCount;
    options
        .AddLongOption('m', "max_failed_job_count")
        .Optional()
        .DefaultValue(0)
        .StoreResult(&maxFailedJobCount);

    TString begemotConfig;
    options
        .AddLongOption("begemot_config", "begemot_config item in apphost context")
        .Optional()
        .RequiredArgument("BEGEMOT_CONFIG")
        .StoreResult(&begemotConfig);

    TYtTokenCommandLineOption token(
        options,
        "39d53ed3928440fbbc1074e727ddcd97",
        "536f6f3da86e4a47b5ef74ffe0b52367"
    );

    TYtProxyCommandLineOption proxy(options);

    TMaybe<TString> stderrTable;
    options
        .AddLongOption("stderr_table", "stderr_table_path in spec")
        .Optional()
        .RequiredArgument("STDERR_TABLE")
        .StoreResultT<TString>(&stderrTable);

    TMaybe<int> pipeFd;
    options
        .AddLongOption("pipe_fd", "pipe file descriptor to write operation id and exit without waiting")
        .Optional()
        .RequiredArgument("PIPE_FD")
        .StoreResultT<int>(&pipeFd);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    while(appendCgi.StartsWith("&")) {
        appendCgi = appendCgi.substr(1);
    }

    Y_VERIFY(shardPaths.Defined(), "Cypress shard parameter not found");
    Y_VERIFY(directMode.Defined() || !shardName.Empty(), "Shard name must be provided in non-direct mode");

    NYT::TNode commonSpec = NYT::TNode();

    IClientPtr client = NYT::CreateClient(proxy.Get(),
        NYT::TCreateClientOptions().Token(token.Get()));
    NYT::TConfig::Get()->ApiFilePathOptions = TRichYPath().BypassArtifactCache(true);

    EBegemotMode mode = directMode.Defined() ? EBegemotMode::Direct :
                        (cgiMode ? EBegemotMode::Cgi : EBegemotMode::Normal);

    auto direct = directMode.Get(inputTable, client);

    auto spec = NYT::TNode()
        ("title", Join(" ", "Begemot mapper", shardName.Empty() ? "Direct" :  shardName))
        ("mapper", NYT::TNode()
            ("memory_reserve_factor", 1.0)
            ("cpu_limit", threadsCount + 1)
        );

    auto jobSpec = TUserJobSpec();

    TVector<TString> selectedRules;
    SelectAllRulesByRequiredRules(requiredRules, selectedRules);

    TVector<TString> nodeShardFiles;
    auto shard = shardPaths.Get(client, selectedRules);
    for (auto& file : shard.Files) {
        jobSpec.AddFile(TRichYPath(TString::Join("<bypass_artifact_cache = %true>", file.CypressPath)).FileName(file.LocalName));
        nodeShardFiles.push_back(file.LocalName);
    }

    TVector<TString> nodeFreshFiles;
    auto fresh = freshPaths.Get(client, selectedRules);
    for (auto& file : fresh.Files) {
        jobSpec.AddFile(TRichYPath(TString::Join("<bypass_artifact_cache = %true>", file.CypressPath)).FileName(file.LocalName));
        nodeFreshFiles.push_back(file.LocalName);
    }

    ui64 shardSize = shard.TotalSize + fresh.TotalSize;
    jobSpec.MemoryLimit(shardSize * 1.4 + 15 * GB); // shard is bigger after loading into begemot
    jobSpec.ExtraTmpfsSize(15 * MB);
    Cerr << "Shard size: " << shardSize << Endl;

    auto richOutputPath = TRichYPath(outputTable).Schema(
        directMode.Defined() || shardName == "Merger" ? MERGER_SCHEMA : WORKER_SCHEMA);
    if (rowCountLimit.Defined()) {
        richOutputPath.RowCountLimit(*rowCountLimit);
    }

    TString coredumpPath = outputTable + ".core";
    client->Create(coredumpPath, NYT::ENodeType::NT_TABLE, TCreateOptions().Force(true).Recursive(true));

    TMapOperationSpec mapSpec = TMapOperationSpec()
        .AddInput<TNode>(inputTable)
        .AddOutput<TNode>(richOutputPath)
        .AddOutput<TNode>(outputTable + ".bad_requests")
        .AddOutput<TNode>(outputTable + ".error")
        .AddOutput<TNode>(outputTable + ".stat")
        .MapperSpec(jobSpec)
        .CoreTablePath(coredumpPath)
        .MaxFailedJobCount(maxFailedJobCount);

    if (stderrTable.Defined()) {
        mapSpec.StderrTablePath(stderrTable.GetRef());
    }
    if (jobCountConfig.Defined()) {
        mapSpec.JobCount(jobCountConfig.Get(inputTable, client));
    }

    auto operation = client->Map(
        mapSpec,
        new TBegemotMapper(nodeShardFiles, nodeFreshFiles,
            threadsCount, cacheSize, mode, begemotConfig, shardName,
            direct.QueryColumn, direct.RegionColumnOrValue, direct.RegionIsValue,
            appendCgi),
        TOperationOptions()
            .MountSandboxInTmpfs(true)
            .Spec(spec)
            .Wait(!pipeFd.Defined())
    );
    if (pipeFd.Defined()) {
        auto opId = GetGuidAsString(operation->GetId());
        if (-1 == write(pipeFd.GetRef(), opId.data(), opId.size())) {
            perror("Cannot write operation id");
        }
    }

    return 0;
}
