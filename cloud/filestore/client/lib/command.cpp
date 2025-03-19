#include "command.h"

#include <cloud/filestore/libs/client/durable.h>
#include <cloud/filestore/libs/client/probes.h>
#include <cloud/filestore/libs/fuse/probes.h>

#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>

#include <library/cpp/lwtrace/mon/mon_lwtrace.h>

#include <util/datetime/base.h>
#include <util/generic/guid.h>

namespace NCloud::NFileStore::NClient {

using namespace NThreading;

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration WaitTimeout = TDuration::Seconds(1);

////////////////////////////////////////////////////////////////////////////////

TCommand::TCommand()
{
    Opts.AddHelpOption('h');
    Opts.AddVersionOption();

    Opts.AddLongOption("verbose")
        .OptionalArgument("STR")
        .StoreResult(&VerboseLevel);

    Opts.AddLongOption("mon-address")
        .RequiredArgument("STR")
        .StoreResult(&MonitoringAddress);

    Opts.AddLongOption("mon-port")
        .RequiredArgument("NUM")
        .StoreResult(&MonitoringPort);

    Opts.AddLongOption("mon-threads")
        .RequiredArgument("NUM")
        .StoreResult(&MonitoringThreads);

    Opts.AddLongOption("server-address")
        .RequiredArgument("STR")
        .StoreResult(&ServerAddress);

    Opts.AddLongOption("server-port")
        .RequiredArgument("NUM")
        .StoreResult(&ServerPort);
}

int TCommand::Run(int argc, char** argv)
{
    OptsParseResult.ConstructInPlace(&Opts, argc, argv);

    Init();
    Start();

    if (!Execute()) {
        // wait until operation completed
        with_lock (WaitMutex) {
            while (ProgramShouldContinue.PollState() == TProgramShouldContinue::Continue) {
                WaitCondVar.WaitT(WaitMutex, WaitTimeout);
            }
        }
    }

    Stop();

    return ProgramShouldContinue.GetReturnCode();
}

void TCommand::Stop(int exitCode)
{
    ProgramShouldContinue.ShouldStop(exitCode);
    WaitCondVar.Signal();
}

bool TCommand::WaitForI(const TFuture<void>& future)
{
    while (ProgramShouldContinue.PollState() == TProgramShouldContinue::Continue) {
        if (future.Wait(WaitTimeout)) {
            return true;
        }
    }
    return false;
}

void TCommand::Init()
{
    LogSettings.UseLocalTimestamps = true;

    if (OptsParseResult->FindLongOptParseResult("verbose") && !VerboseLevel) {
        VerboseLevel = "debug";
    }

    if (VerboseLevel) {
        auto level = GetLogLevel(VerboseLevel);
        Y_ENSURE(level, "unknown log level: " << VerboseLevel.Quote());

        LogSettings.FiltrationLevel = *level;
    }

    Logging = CreateLoggingService("console", LogSettings);
    Log = Logging->CreateLog("NFS_CLIENT");

    if (MonitoringPort) {
        Monitoring = CreateMonitoringService(
            MonitoringPort,
            MonitoringAddress,
            MonitoringThreads);
    } else {
        Monitoring = CreateMonitoringServiceStub();
    }

    auto& probes = NLwTraceMonPage::ProbeRegistry();
    probes.AddProbesList(LWTRACE_GET_PROBES(FILESTORE_FUSE_PROVIDER));
    probes.AddProbesList(LWTRACE_GET_PROBES(FILESTORE_CLIENT_PROVIDER));

    Timer = CreateWallClockTimer();
    Scheduler = CreateScheduler();

    NProto::TClientConfig config;
    if (ServerAddress) {
        config.SetHost(ServerAddress);
    }
    if (ServerPort) {
        config.SetPort(ServerPort);
    }

    ClientConfig = std::make_shared<TClientConfig>(config);
}

void TCommand::Start()
{
    if (Scheduler) {
        Scheduler->Start();
    }

    if (Logging) {
        Logging->Start();
    }

    if (Monitoring) {
        Monitoring->Start();
    }
}

void TCommand::Stop()
{
    if (Monitoring) {
        Monitoring->Stop();
    }

    if (Logging) {
        Logging->Stop();
    }

    if (Scheduler) {
        Scheduler->Stop();
    }
}

////////////////////////////////////////////////////////////////////////////////

void TFileStoreServiceCommand::Init()
{
    TCommand::Init();

    Client = CreateDurableClient(
        Logging,
        Timer,
        Scheduler,
        CreateRetryPolicy(ClientConfig),
        CreateFileStoreClient(ClientConfig, Logging));
}

void TFileStoreServiceCommand::Start()
{
    TCommand::Start();

    if (Client) {
        Client->Start();
    }
}

void TFileStoreServiceCommand::Stop()
{
    if (Client) {
        Client->Stop();
    }

    TCommand::Stop();
}

TFileStoreCommand::TFileStoreCommand()
{
    ClientId = CreateGuidAsString();

    Opts.AddLongOption("filesystem")
        .Required()
        .RequiredArgument("STR")
        .StoreResult(&FileSystemId);
}

////////////////////////////////////////////////////////////////////////////////

void TFileStoreCommand::Start()
{
    TFileStoreServiceCommand::Start();
}

void TFileStoreCommand::Stop()
{
    TFileStoreServiceCommand::Stop();
}

void TFileStoreCommand::CreateSession()
{
    // TODO use ISession instead
    auto request = std::make_shared<NProto::TCreateSessionRequest>();
    request->SetFileSystemId(FileSystemId);
    request->MutableHeaders()->SetClientId(ClientId);
    request->SetRestoreClientSession(true);

    TCallContextPtr ctx = MakeIntrusive<TCallContext>();
    auto response = WaitFor(Client->CreateSession(ctx, std::move(request)));
    CheckResponse(response);

    auto sessionId = response.GetSession().GetSessionId();

    Headers.SetClientId(ClientId);
    Headers.SetSessionId(sessionId);
}

////////////////////////////////////////////////////////////////////////////////

NProto::TNodeAttr TFileStoreCommand::ResolveNode(
    ui64 parentNodeId,
    TString name,
    bool ignoreMissing)
{
    const auto invalidNodeId = Max<ui64>();

    auto makeInvalidNode = [&] () {
        NProto::TNodeAttr node;
        node.SetType(NProto::E_INVALID_NODE);   // being explicit about the type
        node.SetId(invalidNodeId);
        return node;
    };

    if (parentNodeId == invalidNodeId) {
        return makeInvalidNode();
    }

    auto request = CreateRequest<NProto::TGetNodeAttrRequest>();
    request->SetNodeId(parentNodeId);
    request->SetName(std::move(name));

    auto response = WaitFor(Client->GetNodeAttr(
        PrepareCallContext(),
        std::move(request)));

    const auto code = MAKE_FILESTORE_ERROR(NProto::E_FS_NOENT);
    if (ignoreMissing && response.GetError().GetCode() == code) {
        return makeInvalidNode();
    }

    CheckResponse(response);

    return response.GetNode();
}

TVector<TFileStoreCommand::TPathEntry> TFileStoreCommand::ResolvePath(
    TStringBuf path,
    bool ignoreMissing)
{
    TStringBuf tok;
    TStringBuf it(path);

    TVector<TPathEntry> result;
    result.emplace_back();
    result.back().Node.SetId(RootNodeId);
    result.back().Node.SetType(NProto::E_DIRECTORY_NODE);

    while (it.NextTok('/', tok)) {
        if (tok) {
            auto node = ResolveNode(
                result.back().Node.GetId(),
                ToString(tok),
                ignoreMissing
            );
            result.push_back({node, tok});
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////

TEndpointCommand::TEndpointCommand()
{
}

void TEndpointCommand::Init()
{
    TCommand::Init();

    Client = CreateDurableClient(
        Logging,
        Timer,
        Scheduler,
        CreateRetryPolicy(ClientConfig),
        CreateEndpointManagerClient(ClientConfig, Logging));
}

void TEndpointCommand::Start()
{
    TCommand::Start();

    if (Client) {
        Client->Start();
    }
}

void TEndpointCommand::Stop()
{
    if (Client) {
        Client->Stop();
    }

    TCommand::Stop();
}

}   // namespace NCloud::NFileStore::NClient
