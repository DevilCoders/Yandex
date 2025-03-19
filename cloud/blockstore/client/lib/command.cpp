#include "command.h"
#include "factory.h"

#include <cloud/blockstore/libs/client/client.h>
#include <cloud/blockstore/libs/client/config.h>
#include <cloud/blockstore/libs/client/durable.h>
#include <cloud/blockstore/libs/client/session.h>
#include <cloud/blockstore/libs/client/throttling.h>
#include <cloud/blockstore/libs/diagnostics/probes.h>
#include <cloud/blockstore/libs/diagnostics/request_stats.h>
#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/encryption/encryption_client.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/service.h>
#include <cloud/blockstore/libs/throttling/throttler_logger.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>
#include <cloud/storage/core/libs/diagnostics/stats_updater.h>
#include <cloud/storage/core/libs/grpc/executor.h>
#include <cloud/storage/core/libs/grpc/logging.h>
#include <cloud/storage/core/libs/grpc/threadpool.h>
#include <cloud/storage/core/libs/version/version.h>

#include <library/cpp/lwtrace/mon/mon_lwtrace.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/folder/dirut.h>
#include <util/generic/guid.h>
#include <util/stream/file.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/strip.h>
#include <util/string/subst.h>
#include <util/system/fs.h>
#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NClient {

namespace {

////////////////////////////////////////////////////////////////////////////////

const TString DefaultConfigFile = "/Berkanavt/nbs-server/cfg/nbs-client.txt";
const TString DefaultIamTokenFile = "~/.nbs-client/iam-token";

////////////////////////////////////////////////////////////////////////////////

static const TMap<TString, NProto::EEncryptionMode> EncryptionModes = {
    { "no",         NProto::NO_ENCRYPTION       },
    { "aes-xts",    NProto::ENCRYPTION_AES_XTS  },
    { "test",       NProto::ENCRYPTION_TEST     },
};

////////////////////////////////////////////////////////////////////////////////

TString ResolvePath(const TString& path)
{
    if (path.StartsWith('~')) {
        return TStringBuilder() << GetHomeDir() << path.substr(1);
    }

    return path;
}

TString GetIamToken(const TString& iamTokenFile)
{
    auto filename = ResolvePath(iamTokenFile);
    TFile file;
    try {
        file = TFile(filename, EOpenModeFlag::OpenExisting | EOpenModeFlag::RdOnly);
    } catch (...) {
        return {};
    }

    if (!file.IsOpen()) {
        return {};
    }

    return Strip(TFileInput(file).ReadAll());
}

}   // namespace

using namespace NLastGetopt;

////////////////////////////////////////////////////////////////////////////////

TCommand::TCommand(IBlockStorePtr client)
    : ClientEndpoint(std::move(client))
{
    Opts.AddHelpOption('h');
    Opts.SetFreeArgsNum(0);

    Opts.AddLongOption("config")
        .Help(TStringBuilder()
            << "config file name. Default is "
            << DefaultConfigFile)
        .RequiredArgument("STR")
        .StoreResult(&ConfigFile);

    Opts.AddLongOption("host", "connect host")
        .RequiredArgument("STR")
        .StoreResult(&Host);

    Opts.AddLongOption("port", "connect port")
        .RequiredArgument("NUM")
        .StoreResult(&InsecurePort);

    Opts.AddLongOption("secure-port", "connect secure port (overrides --port)")
        .RequiredArgument("NUM")
        .StoreResult(&SecurePort);

    Opts.AddLongOption("mon-file")
        .RequiredArgument("STR")
        .StoreResult(&MonitoringConfig);

    Opts.AddLongOption("mon-address")
        .RequiredArgument("STR")
        .StoreResult(&MonitoringAddress);

    Opts.AddLongOption("mon-port")
        .RequiredArgument("NUM")
        .StoreResult(&MonitoringPort);

    Opts.AddLongOption("mon-threads")
        .RequiredArgument("NUM")
        .StoreResult(&MonitoringThreads);

    Opts.AddLongOption("verbose", "output level for diagnostics messages")
        .OptionalArgument("STR")
        .StoreResult(&VerboseLevel);

    Opts.AddLongOption("timeout", "timeout in seconds")
        .OptionalArgument("NUM")
        .Handler1T<ui32>([this] (const auto& timeout) {
            Timeout = TDuration::Seconds(timeout);
        });

    Opts.AddLongOption("proto")
        .Help("Use protobuf syntax for requests and responses")
        .NoArgument()
        .SetFlag(&Proto);

    Opts.AddLongOption("input", "input file name (or stdin if not specified)")
        .RequiredArgument("STR")
        .StoreResult(&InputFile);

    Opts.AddLongOption("error", "error file name (or stderr if not specified)")
        .RequiredArgument("STR")
        .StoreResult(&ErrorFile);

    Opts.AddLongOption("output", "output file name (or stdout if not specified)")
        .RequiredArgument("STR")
        .StoreResult(&OutputFile);

    Opts.AddLongOption("grpc-trace", "turn on grpc tracing")
        .NoArgument()
        .StoreTrue(&EnableGrpcTracing);

    Opts.AddLongOption("iam-token-file", "path to iam token")
        .RequiredArgument("STR")
        .StoreResult(&IamTokenFile);

    Opts.AddLongOption(
        "client-performance-profile",
        "path to client performance profile")
        .RequiredArgument("STR")
        .StoreResult(&ClientPerformanceProfile);
}

TCommand::~TCommand()
{}

void TCommand::Prepare(int argc, const char* argv[])
{
    Parse(argc, argv);
    Init();
}

bool TCommand::Execute()
{
    if (Timeout != TDuration::Zero()) {
        Scheduler->Schedule(
            Timer->Now() + Timeout,
            [=] {
                Shutdown();
            });
    }

    return DoExecute();
}

void TCommand::Shutdown()
{
    ShouldContinue.ShouldStop(1);
}

bool TCommand::Run(const int argc, const char* argv[])
{
    try {
        Prepare(argc, argv);
    } catch (...) {
        // Log might not be set up here so use Cerr
        Cerr << CurrentExceptionMessage() << Endl;
        return false;
    }

    bool res = Execute();

    try {
        Stop();
    } catch (...) {
        STORAGE_ERROR(CurrentExceptionMessage());
        return false;
    }

    return res;
}

IInputStream& TCommand::GetInputStream()
{
    if (InputFile) {
        if (!InputStream) {
            InputStream.reset(new TIFStream(InputFile));
        }
        return *InputStream;
    }
    return Cin;
}

IOutputStream& TCommand::GetErrorStream()
{
    if (ErrorFile) {
        if (!OutputStream) {
            ErrorStream.reset(new TOFStream(ErrorFile));
        }
        return *ErrorStream;
    }
    return Cerr;
}

IOutputStream& TCommand::GetOutputStream()
{
    if (OutputFile) {
        if (!OutputStream) {
            OutputStream.reset(new TOFStream(OutputFile));
        }
        return *OutputStream;
    }
    if (!OutputStream) {
        return Cout;
    }
    return *OutputStream;
}

void TCommand::SetOutputStream(std::shared_ptr<IOutputStream> os)
{
    OutputStream = std::move(os);
}

TString TCommand::NormalizeCommand(TString command)
{
    command.to_lower();
    SubstGlobal(command, "-", TStringBuf{});
    SubstGlobal(command, "_", TStringBuf{});
    return command;
}

NProto::EEncryptionMode TCommand::EncryptionModeFromString(const TString& str)
{
    auto it = EncryptionModes.find(str);
    if (it != EncryptionModes.end()) {
        return it->second;
    }

    ythrow yexception() << "invalid encryption mode: " << str;
}

NProto::TEncryptionSpec TCommand::CreateEncryptionSpec(
    NProto::EEncryptionMode mode,
    const TString& keyPath,
    const TString& keyHash)
{
    if (!keyHash.empty()) {
        if (mode != NProto::NO_ENCRYPTION || !keyPath.empty()) {
            throw yexception() << "invalid encryption options: "
                << " set only (key hash) or (key path + mode)";
        }

        NProto::TEncryptionSpec encryptionSpec;
        encryptionSpec.SetKeyHash(keyHash);
        encryptionSpec.SetDisableEncryption(true);
        return encryptionSpec;
    }

    if (!keyPath.empty()) {
        if (mode == NProto::NO_ENCRYPTION) {
            throw yexception() << "invalid encryption options: "
                << " set only (key hash) or (key path + mode)";
        }

        NProto::TEncryptionSpec encryptionSpec;
        auto& encryptionKey = *encryptionSpec.MutableKey();
        encryptionKey.SetMode(mode);
        encryptionKey.MutableKeyPath()->SetFilePath(keyPath);
        return encryptionSpec;
    }

    if (mode != NProto::NO_ENCRYPTION) {
        throw yexception() << "invalid encryption options: "
            << " set only (key hash) or (key path + mode)";
    }

    return {};
}

NProto::TMountVolumeResponse TCommand::MountVolume(
    TString diskId,
    TString mountToken,
    ISessionPtr& session,
    NProto::EVolumeAccessMode accessMode,
    bool mountLocal,
    bool throttlingDisabled,
    const NProto::TEncryptionSpec& encryptionSpec)
{
    TSessionConfig sessionConfig;
    sessionConfig.DiskId = std::move(diskId);
    sessionConfig.MountToken = std::move(mountToken);
    sessionConfig.MountMode = mountLocal
        ? NProto::VOLUME_MOUNT_LOCAL : NProto::VOLUME_MOUNT_REMOTE;
    if (throttlingDisabled) {
        SetProtoFlag(sessionConfig.MountFlags, NProto::MF_THROTTLING_DISABLED);
    }
    sessionConfig.ClientVersionInfo = GetFullVersionString();
    sessionConfig.AccessMode = accessMode;

    auto endpointOrError = TryToCreateEncryptionClient(
        ClientEndpoint,
        Logging,
        encryptionSpec);

    if (HasError(endpointOrError)) {
        return TErrorResponse(endpointOrError.GetError());
    }
    const auto& endpoint = endpointOrError.GetResult();

    session = CreateSession(
        Timer,
        Scheduler,
        Logging,
        RequestStats,
        VolumeStats,
        endpoint,
        ClientConfig,
        sessionConfig);

    auto response = SafeExecute<NProto::TMountVolumeResponse>([&] {
        return WaitFor(session->MountVolume());
    });

    if (HasError(response)) {
        STORAGE_ERROR(
            "Failed to mount volume: " << FormatError(response.GetError()));
    }
    return response;
}

bool TCommand::UnmountVolume(ISession& session)
{
    auto response = SafeExecute<NProto::TUnmountVolumeResponse>([&] {
        return WaitFor(session.UnmountVolume());
    });

    if (HasError(response)) {
        STORAGE_ERROR(
            "Failed to unmount volume: " << FormatError(response.GetError()));
        return false;
    }
    return true;
}

bool TCommand::WaitForI(const NThreading::TFuture<void>& future)
{
    while (ShouldContinue.PollState() == TProgramShouldContinue::Continue) {
        if (future.Wait(WaitTimeout)) {
            return true;
        }
    }
    return false;
}

void TCommand::Parse(const int argc, const char* argv[])
{
    ParseResultPtr = std::make_unique<TOptsParseResultException>(&Opts, argc, argv);

    if (ParseResultPtr->FindLongOptParseResult("verbose") && !VerboseLevel) {
        VerboseLevel = "debug";
    }
}

void TCommand::Init()
{
    InitLWTrace();
    InitClientConfig();

    Timer = CreateWallClockTimer();
    Scheduler = CreateScheduler();

    const auto& logConfig = ClientConfig->GetLogConfig();
    const auto& monConfig = ClientConfig->GetMonitoringConfig();

    TLogSettings logSettings;
    logSettings.UseLocalTimestamps = true;

    if (logConfig.HasLogLevel()) {
        logSettings.FiltrationLevel = static_cast<ELogPriority>(
            logConfig.GetLogLevel());
    }

    Logging = CreateLoggingService("console", logSettings);
    GrpcLog = Logging->CreateLog("GRPC");
    Log = Logging->CreateLog("BLOCKSTORE_CLIENT");

    GrpcLoggerInit(
        GrpcLog,
        logConfig.GetEnableGrpcTracing());

    ui32 maxThreads = ClientConfig->GetGrpcThreadsLimit();
    SetExecutorThreadsLimit(maxThreads);
    SetDefaultThreadPoolLimit(maxThreads);

    ui32 monPort = monConfig.GetPort();
    if (monPort) {
        const TString& monAddress = monConfig.GetAddress();
        ui32 threadsCount = monConfig.GetThreadsCount();
        Monitoring = CreateMonitoringService(monPort, monAddress, threadsCount);
    } else {
        Monitoring = CreateMonitoringServiceStub();
    }

    auto rootGroup = Monitoring->GetCounters()
        ->GetSubgroup("counters", "blockstore");

    auto clientGroup = rootGroup->GetSubgroup("component", "client");
    auto versionCounter = clientGroup->GetNamedCounter(
        "version",
        GetFullVersionString(),
        false);
    *versionCounter = 1;

    RequestStats = CreateClientRequestStats(clientGroup, Timer);

    VolumeStats = CreateVolumeStats(
        Monitoring,
        {},
        EVolumeStatsType::EClientStats,
        Timer);

    if (!ClientEndpoint) {
        ClientStats = CreateClientStats(
            ClientConfig,
            Monitoring,
            RequestStats,
            VolumeStats,
            ClientConfig->GetInstanceId());

        auto [client, error] = CreateClient(
            ClientConfig,
            Timer,
            Scheduler,
            Logging,
            Monitoring,
            ClientStats);

        Y_VERIFY(!HasError(error));
        Client = std::move(client);

        StatsUpdater = CreateStatsUpdater(
            Timer,
            Scheduler,
            ClientStats,
            {}  // TODO: fill incompleteRequestProviders (NBS-2167)
        );

        auto endpoint = Client->CreateEndpoint();

        auto retryPolicy = CreateRetryPolicy(ClientConfig);

        ClientEndpoint = CreateDurableClient(
            ClientConfig,
            std::move(endpoint),
            std::move(retryPolicy),
            Logging,
            Timer,
            Scheduler,
            RequestStats,
            VolumeStats);

        if (ClientPerformanceProfile) {
            NProto::TClientPerformanceProfile performanceProfile;
            ParseFromTextFormat(ClientPerformanceProfile, performanceProfile);
            ClientEndpoint = CreateThrottlingClient(
                std::move(ClientEndpoint),
                CreateThrottler(
                    CreateThrottlerLoggerDefault(
                        RequestStats,
                        Logging,
                        "BLOCKSTORE_CLIENT"),
                    CreateThrottlerMetrics(
                        Timer,
                        rootGroup,
                        Logging,
                        ClientConfig->GetInstanceId().empty()
                            ? ClientConfig->GetClientId()
                            : ClientConfig->GetInstanceId()),
                    CreateThrottlerPolicy(performanceProfile),
                    CreateThrottlerTracker(),
                    Timer,
                    Scheduler,
                    VolumeStats));
        }

    }

    Start();
}

void TCommand::InitLWTrace()
{
    auto& probes = NLwTraceMonPage::ProbeRegistry();
    probes.AddProbesList(LWTRACE_GET_PROBES(BLOCKSTORE_SERVER_PROVIDER));
}

void TCommand::InitClientConfig()
{
    NProto::TClientAppConfig appConfig;
    if (ConfigFile) {
        ParseFromTextFormat(ConfigFile, appConfig);
    } else if (NFs::Exists(DefaultConfigFile)) {
        ParseFromTextFormat(DefaultConfigFile, appConfig);
    }

    auto& clientConfig = *appConfig.MutableClientConfig();
    if (Host) {
        clientConfig.SetHost(Host);
    }
    if (InsecurePort) {
        clientConfig.SetInsecurePort(InsecurePort);
    }
    if (SecurePort) {
        clientConfig.SetSecurePort(SecurePort);
    }
    if (clientConfig.GetHost() == "localhost" &&
        clientConfig.GetSecurePort() != 0)
    {
        // With TLS on transform localhost into fully qualified domain name.
        clientConfig.SetHost(FQDNHostName());
    }
    // AuthToken is set up using AuthConfig.
    clientConfig.ClearAuthToken();

    auto& monConfig = *appConfig.MutableMonitoringConfig();
    // Monitoring is set up via command line.
    monConfig.Clear();
    if (MonitoringConfig) {
        ParseFromTextFormat(MonitoringConfig, monConfig);
    }
    if (MonitoringAddress) {
        monConfig.SetAddress(MonitoringAddress);
    }
    if (MonitoringPort) {
        monConfig.SetPort(MonitoringPort);
    }
    if (MonitoringThreads) {
        monConfig.SetThreadsCount(MonitoringThreads);
    }
    if (!monConfig.GetThreadsCount()) {
        monConfig.SetThreadsCount(1);  // reasonable defaults
    }

    auto& logConfig = *appConfig.MutableLogConfig();
    // Logging is set up via command line.
    logConfig.Clear();
    logConfig.SetEnableGrpcTracing(EnableGrpcTracing);
    if (VerboseLevel) {
        auto level = GetLogLevel(VerboseLevel);
        if (!level) {
            ythrow yexception()
                << "unknown log level: " << VerboseLevel.Quote();
        }
        logConfig.SetLogLevel(*level);
    }

    if (!IamTokenFile) {
        auto& authConfig = appConfig.GetAuthConfig();
        if (authConfig.HasIamTokenFile()) {
            IamTokenFile = authConfig.GetIamTokenFile();
        } else {
            IamTokenFile = DefaultIamTokenFile;
        }
    }

    // Do not send token via insecure channel.
    if (clientConfig.GetSecurePort() != 0) {
        clientConfig.SetAuthToken(GetIamToken(IamTokenFile));
    }

    if (!clientConfig.GetClientId()) {
        clientConfig.SetClientId(CreateGuidAsString());
    }

    ClientConfig = std::make_shared<TClientAppConfig>(appConfig);
}

void TCommand::Start()
{
    if (Logging) {
        Logging->Start();
    }

    if (Monitoring) {
        Monitoring->Start();
    }

    if (Client) {
        Client->Start();
    }

    if (StatsUpdater) {
        StatsUpdater->Start();
    }

    if (ClientEndpoint) {
        ClientEndpoint->Start();
    }

    if (Scheduler) {
        Scheduler->Start();
    }
}

void TCommand::Stop()
{
    if (Scheduler) {
        Scheduler->Stop();
    }

    if (ClientEndpoint) {
        ClientEndpoint->Stop();
    }

    if (StatsUpdater) {
        StatsUpdater->Stop();
    }

    if (Client) {
        Client->Stop();
    }

    if (Monitoring) {
        Monitoring->Stop();
    }

    if (Logging) {
        Logging->Stop();
    }
}

}   // namespace NCloud::NBlockStore::NClient
