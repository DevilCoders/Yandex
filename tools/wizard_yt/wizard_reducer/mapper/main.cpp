#include "processrow_task.h"

#include <tools/wizard_yt/shard_packer/shard_packer.h>
#include <tools/wizard_yt/reducer_common/common.h>
#include <tools/wizard_yt/reducer_common/cmd_line_options.h>
#include <tools/wizard_yt/reducer_common/rule_updater.h>
#include <tools/printwzrd/lib/options.h>
#include <tools/printwzrd/lib/prepare_request.h>
#include <tools/printwzrd/lib/printer.h>

#include <search/wizard/wizard.h>
#include <search/wizard/config/config.h>
#include <search/wizard/core/data_loader.h>
#include <search/wizard/face/rules_names.h>
#include <mapreduce/yt/interface/common.h>

#include <web/daemons/wizard/wizard_main/wizard.h>

#include <util/system/env.h>
#include <util/generic/guid.h>


class TWizardMapper : public TBegemotMapperBase {
    TVector<TString> ShardFiles;
    bool DumpEventLog;
    TPrintwzrdOptions InitOptions;
    TString ExtractShardPrefix;
    THolder<TReqWizard> Wizard;
    TEventLogPtr EventLogPtr;
    TThreadPool Mtp;
    THolder<TSerialPostProcessQueue> Queue;
public:
    Y_SAVELOAD_JOB(DumpEventLog, ExtractShardPrefix, ShardFiles, InitOptions);

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        SetEnv("MKL_CBWR", "COMPATIBLE");
        ExtractShard(ShardFiles, "wizard/WIZARD_SHARD");
        using namespace NPrintWzrd;
        Y_VERIFY(InitOptions.OkLocal(), "Yt version only works in local mode");

        TAutoPtr<TWizardConfig> cfg = TWizardConfig::CreateWizardConfig(InitOptions.PathToConfig.data(), InitOptions.WorkDir.data());
        cfg->SetCollectingRuntimeStat(false);
        cfg->SetWizardThreadCount(InitOptions.ThreadCount);

        auto factory = NEvClass::Factory();
        TEventLogPtr eventLogPtr = DumpEventLog ? new TEventLog("/dev/null", factory->CurrentFormat()) : new TEventLog(0);
        Wizard.Reset(CreateRequestWizard(*cfg, eventLogPtr).Release());
        Y_VERIFY(Wizard.Get() != nullptr);

        Queue.Reset(new TSerialPostProcessQueue(&Mtp));
        Queue->Start(InitOptions.ThreadCount, 1024);
    }

    void Finish(NYT::TTableWriter<NYT::TNode>* writer) override {
        Queue->Stop();
        TStringStream cacheStr;
        Wizard->ReportCacheStats(cacheStr);
        NYT::TNode cacheRow = NYT::TNode::CreateMap();
        cacheRow["cache_stat"] = cacheStr.Str();
        writer->AddRow(cacheRow, 2);
    }

    void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            TSelfFlushLogFramePtr framePtr = DumpEventLog ? MakeIntrusive<TSelfFlushLogFrame>(*EventLogPtr) : nullptr;
            TAutoPtr<TProcessRowTask> task(new TProcessRowTask(Wizard.Get(), reader->GetRow(), InitOptions, framePtr, writer));
            Queue->SafeAdd(task.Release());
        }
    }

    TWizardMapper() {
    }

    TWizardMapper(const TVector<TString>& wizardFiles, bool dumpEventLog, const TPrintwzrdOptions& initOptions)
        : ShardFiles(wizardFiles)
        , DumpEventLog(dumpEventLog)
        , InitOptions(initOptions)
    {
    }
};

REGISTER_MAPPER(TWizardMapper);


int main(int argc, char *argv[]) {
    NYT::Initialize(argc, (const char **)argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(NYT::ILogger::INFO));

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

    TString configFilePath;
    options
        .AddLongOption('y', "config_file", "Path to wizard config file")
        .RequiredArgument("CONFIG")
        .Required()
        .StoreResult(&configFilePath);

    TMaybe<int> pipeFd;
    options
        .AddLongOption("pipe_fd", "pipe file descriptor to write operation id and exit without waiting")
        .Optional()
        .RequiredArgument("PIPE_FD")
        .StoreResultT<int>(&pipeFd);

    bool fullOutput = false;
    options
        .AddLongOption('u', "print_full_output", "Print full output")
        .SetFlag(&fullOutput)
        .NoArgument()
        .Optional();

    bool dumpEventLog;
    options
        .AddLongOption('c', "save_eventlog", "Save eventlog for jobs")
        .SetFlag(&dumpEventLog)
        .NoArgument()
        .Optional();

    TMaybe<i64> rowCountLimit;
    options
        .AddLongOption('l', "row_count_limit", "sets row_count_limit on final Map operation")
        .Optional()
        .RequiredArgument("LIMIT")
        .StoreResultT<i64>(&rowCountLimit);

    i64 threadsCount;
    options
        .AddLongOption('t', "threads", "threads count")
        .Optional()
        .RequiredArgument("JOBS")
        .DefaultValue(16)
        .StoreResult(&threadsCount);

    TVector<TString> requiredRules;
    options
        .AddLongOption("rule", "Rule, selected for execution")
        .Optional()
        .RequiredArgument("RULE")
        .AppendTo(&requiredRules);

    TDataFilesCommandLineOptions shardPaths("shard", options);

    TJobCountConfigCommandLineOptions jobCount(options);

    ui64 maxFailedJobCount;
    options
        .AddLongOption("max_failed_job_count")
        .Optional()
        .DefaultValue(0)
        .StoreResult(&maxFailedJobCount);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    NYT::IClientPtr client = NYT::CreateClient(GetEnv("YT_PROXY"));

    TPrintwzrdOptions initOptions;
    initOptions.PrintRichTree = fullOutput;
    initOptions.PrintQtree = true;
    initOptions.PrintCgiParamRelev = true;
    initOptions.SortOutput = true;
    initOptions.PrintExtraOutput = false;
    initOptions.PrintSrc = false;
    initOptions.ThreadCount = threadsCount;
    initOptions.WorkDir = "./";
    initOptions.PathToConfig = TFsPath(configFilePath).GetName();

    auto spec = NYT::TNode()
        ("title", "Wizard mapper")
        ("mapper", NYT::TNode()
            ("memory_reserve_factor", 1.0)
            ("cpu_limit", threadsCount + 1)
        );

    auto jobSpec = NYT::TUserJobSpec();
    TVector<TString> selectedRules;
    SelectAllRulesByRequiredRules(requiredRules, selectedRules);
    auto shard = shardPaths.Get(client, selectedRules);
    TVector<TString> nodeFiles;
    for (auto& file : shard.Files) {
        jobSpec.AddFile(NYT::TRichYPath(file.CypressPath).FileName(file.LocalName));
        nodeFiles.push_back(file.LocalName);
    }
    jobSpec.AddLocalFile(configFilePath);
    jobSpec.MemoryLimit(shard.TotalSize + 1 * GB);
    jobSpec.ExtraTmpfsSize(20 * MB);
    Cerr << "Shard size: " << shard.TotalSize << Endl;

    auto richOutputPath = NYT::TRichYPath(outputTable);
    if (rowCountLimit.Defined()) {
        richOutputPath.RowCountLimit(*rowCountLimit);
    }

    TString coredumpPath = outputTable + ".core";
    client->Create(coredumpPath, NYT::ENodeType::NT_TABLE, NYT::TCreateOptions().IgnoreExisting(true).Recursive(true));

    NYT::TMapOperationSpec mapSpec = NYT::TMapOperationSpec()
        .AddInput<NYT::TNode>(inputTable)
        .AddOutput<NYT::TNode>(richOutputPath)
        .AddOutput<NYT::TNode>(outputTable + ".bad_requests")
        .AddOutput<NYT::TNode>(outputTable + ".cache")
        .AddOutput<NYT::TNode>(outputTable + ".error")
        .MapperSpec(jobSpec)
        .CoreTablePath(coredumpPath)
        .MaxFailedJobCount(maxFailedJobCount);

    if (jobCount.Defined()) {
        mapSpec.JobCount(jobCount.Get(inputTable, client));
    }

    auto operation = client->Map(
        mapSpec,
        new TWizardMapper(nodeFiles, dumpEventLog, initOptions),
        NYT::TOperationOptions()
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
