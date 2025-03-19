#include "bootstrap.h"

#include "options.h"

#include <cloud/filestore/libs/diagnostics/critical_events.h>
#include <cloud/filestore/libs/diagnostics/profile_log.h>
#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/server/config.h>
#include <cloud/filestore/libs/server/probes.h>
#include <cloud/filestore/libs/server/server.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/service_kikimr/service.h>
#include <cloud/filestore/libs/service_local/config.h>
#include <cloud/filestore/libs/service_local/service.h>
#include <cloud/filestore/libs/service_null/service.h>
#include <cloud/filestore/libs/storage/core/probes.h>

#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/task_queue.h>
#include <cloud/storage/core/libs/common/thread_pool.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/daemon/mlock.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>
#include <cloud/storage/core/libs/diagnostics/stats_updater.h>
#include <cloud/storage/core/libs/diagnostics/trace_processor.h>
#include <cloud/storage/core/libs/diagnostics/trace_serializer.h>
#include <cloud/storage/core/libs/kikimr/actorsystem.h>
#include <cloud/storage/core/libs/version/version.h>

#include <library/cpp/lwtrace/mon/mon_lwtrace.h>
#include <library/cpp/lwtrace/probes.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/stream/file.h>

namespace NCloud::NFileStore::NServer {

using namespace NActors;
using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

const TString TraceLoggerId = "st_trace_logger";
const TString SlowRequestsFilterId = "st_slow_requests_filter";

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void ParseProtoTextFromFile(const TString& fileName, T& dst)
{
    TFileInput in(fileName);
    ParseFromTextFormat(in, dst);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TFileStoreServerBootstrap::TFileStoreServerBootstrap(TOptionsPtr options)
    : TBootstrap{options, "NFS_SERVER", "server"}
    , Options{options}
{}

TFileStoreServerBootstrap::~TFileStoreServerBootstrap()
{}

////////////////////////////////////////////////////////////////////////////////

void TFileStoreServerBootstrap::StartComponents()
{
    FILESTORE_LOG_START_COMPONENT(Logging);
    FILESTORE_LOG_START_COMPONENT(Monitoring);
    FILESTORE_LOG_START_COMPONENT(TraceProcessor);
    FILESTORE_LOG_START_COMPONENT(ActorSystem);
    FILESTORE_LOG_START_COMPONENT(ThreadPool);
    FILESTORE_LOG_START_COMPONENT(BackgroundThreadPool);
    FILESTORE_LOG_START_COMPONENT(ProfileLog);

    FILESTORE_LOG_START_COMPONENT(Service);
    FILESTORE_LOG_START_COMPONENT(Server);
    FILESTORE_LOG_START_COMPONENT(RequestStatsUpdater);

    // we need to start scheduler after all other components for 2 reasons:
    // 1) any component can schedule a task that uses a dependency that hasn't
    // started yet
    // 2) we have loops in our dependencies, so there is no 'correct' starting
    // order
    FILESTORE_LOG_START_COMPONENT(Scheduler);
    FILESTORE_LOG_START_COMPONENT(BackgroundScheduler);
}

void TFileStoreServerBootstrap::StopComponents()
{
    // stopping scheduler before all other components to avoid races between
    // scheduled tasks and shutting down of component dependencies
    FILESTORE_LOG_STOP_COMPONENT(BackgroundScheduler);
    FILESTORE_LOG_STOP_COMPONENT(Scheduler);

    FILESTORE_LOG_STOP_COMPONENT(RequestStatsUpdater);
    FILESTORE_LOG_STOP_COMPONENT(Server);
    FILESTORE_LOG_STOP_COMPONENT(Service);
    FILESTORE_LOG_STOP_COMPONENT(ProfileLog);
    FILESTORE_LOG_STOP_COMPONENT(BackgroundThreadPool);
    FILESTORE_LOG_STOP_COMPONENT(ThreadPool);
    FILESTORE_LOG_STOP_COMPONENT(ActorSystem);
    FILESTORE_LOG_STOP_COMPONENT(TraceProcessor);
    FILESTORE_LOG_STOP_COMPONENT(Monitoring);
    FILESTORE_LOG_STOP_COMPONENT(Logging);
}

TVector<IIncompleteRequestProviderPtr> TFileStoreServerBootstrap::GetIncompleteRequestProviders()
{
    return {
        Server,
        /* TODO
            std::move(sessionManager),
            EndpointService
        */
    };
}

NServer::IServerPtr TFileStoreServerBootstrap::CreateServer()
{
    InitConfig();

    switch (Options->Service) {
        case NDaemon::EServiceKind::Kikimr:
            InitKikimrService();
            break;
        case NDaemon::EServiceKind::Local:
            InitLocalService();
            break;
        case NDaemon::EServiceKind::Null:
            InitNullService();
            break;
    }

    return NServer::CreateServer(
        ServerConfig,
        Logging,
        StatsRegistry->GetServerStats(),
        ProfileLog,
        Service);
}

void TFileStoreServerBootstrap::InitLWTrace()
{
    auto& probes = NLwTraceMonPage::ProbeRegistry();
    probes.AddProbesList(LWTRACE_GET_PROBES(FILESTORE_SERVER_PROVIDER));
    probes.AddProbesList(LWTRACE_GET_PROBES(FILESTORE_STORAGE_PROVIDER));

    auto& lwManager = NLwTraceMonPage::TraceManager(false);

    const TVector<std::tuple<TString, TString>> desc = {
        {"ExecuteRequest",                  "FILESTORE_SERVER_PROVIDER"},
        {"BackgroundTaskStarted_Tablet",    "FILESTORE_STORAGE_PROVIDER"},
    };

    auto traceLog = CreateUnifiedAgentLoggingService(
        Logging,
        DiagnosticsConfig->GetTracesUnifiedAgentEndpoint(),
        DiagnosticsConfig->GetTracesSyslogIdentifier()
    );

    if (auto samplingRate = DiagnosticsConfig->GetSamplingRate()) {
        NLWTrace::TQuery query = ProbabilisticQuery(
            desc,
            samplingRate,
            DiagnosticsConfig->GetLWTraceShuttleCount());
        lwManager.New(TraceLoggerId, query);
        TraceReaders.push_back(CreateTraceLogger(TraceLoggerId, traceLog, "NFS_TRACE"));
    }

    if (auto samplingRate = DiagnosticsConfig->GetSlowRequestSamplingRate()) {
        NLWTrace::TQuery query = ProbabilisticQuery(
            desc,
            samplingRate,
            DiagnosticsConfig->GetLWTraceShuttleCount());
        lwManager.New(SlowRequestsFilterId, query);
        TraceReaders.push_back(CreateSlowRequestsFilter(
            SlowRequestsFilterId,
            traceLog,
            "NFS_TRACE",
            DiagnosticsConfig->GetHDDSlowRequestThreshold(),
            DiagnosticsConfig->GetSSDSlowRequestThreshold(),
            TDuration::Zero()));
    }
}

////////////////////////////////////////////////////////////////////////////////

void TFileStoreServerBootstrap::InitConfig()
{
    if (Options->AppConfig) {
        ParseProtoTextFromFile(Options->AppConfig, AppConfig);
    }

    auto& serverConfig = *AppConfig.MutableServerConfig();
    if (Options->ServerAddress) {
        serverConfig.SetHost(Options->ServerAddress);
    }
    if (Options->ServerPort) {
        serverConfig.SetPort(Options->ServerPort);
    }

    ServerConfig = std::make_shared<TServerConfig>(serverConfig);
}

void TFileStoreServerBootstrap::InitKikimrService()
{
    Y_VERIFY(ActorSystem, "Actor system MUST be initialized to create kikimr filestore");
    Service = CreateKikimrFileStore(ActorSystem);
}

void TFileStoreServerBootstrap::InitLocalService()
{
    auto serviceConfig = std::make_shared<TLocalFileStoreConfig>(
        *AppConfig.MutableLocalServiceConfig());

    ThreadPool = CreateThreadPool("svc", serviceConfig->GetNumThreads());
    Service = CreateLocalFileStore(
        std::move(serviceConfig),
        Timer,
        Scheduler,
        Logging,
        ThreadPool);
}

void TFileStoreServerBootstrap::InitNullService()
{
    Service = CreateNullFileStore();
}

}   // namespace NCloud::NFileStore::NServer
