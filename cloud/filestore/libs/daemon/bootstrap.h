#pragma once

#include "options.h"
#include "public.h"

#include <cloud/filestore/config/server.pb.h>
#include <cloud/filestore/libs/diagnostics/config.h>
#include <cloud/filestore/libs/server/public.h>
#include <cloud/filestore/libs/service/public.h>
#include <cloud/filestore/libs/storage/core/public.h>

#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/diagnostics/public.h>
#include <cloud/storage/core/libs/kikimr/public.h>

#include <library/cpp/actors/util/should_continue.h>
#include <library/cpp/logger/log.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>


namespace NCloud::NFileStore::NDaemon {

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_LOG_START_COMPONENT(c)                                       \
    if (c) {                                                                   \
        c->Start();                                                            \
        STORAGE_INFO("Started " << #c);                                        \
    }                                                                          \
// FILESTORE_LOG_START_COMPONENT

#define FILESTORE_LOG_STOP_COMPONENT(c)                                        \
    if (c) {                                                                   \
        c->Stop();                                                             \
        STORAGE_INFO("Stopped " << #c);                                        \
    }                                                                          \
// FILESTORE_LOG_STOP_COMPONENT

////////////////////////////////////////////////////////////////////////////////

class TBootstrap {
private:
    const TString MetricsComponent;

    TBootstrapOptionsPtr Options;
    ILoggingServicePtr BootstrapLogging;
    TProgramShouldContinue ProgramShouldContinue;

protected:
    TLog Log;

    TDiagnosticsConfigPtr DiagnosticsConfig;

    NServer::IServerPtr Server;

    ITimerPtr Timer;
    ISchedulerPtr Scheduler;
    ISchedulerPtr BackgroundScheduler;
    ILoggingServicePtr Logging;
    IMonitoringServicePtr Monitoring;
    IRequestStatsRegistryPtr StatsRegistry;
    IStatsUpdaterPtr RequestStatsUpdater;
    TVector<ITraceReaderPtr> TraceReaders;
    ITraceProcessorPtr TraceProcessor;
    ITaskQueuePtr BackgroundThreadPool;
    IProfileLogPtr ProfileLog;

    IActorSystemPtr ActorSystem;
    NKikimrConfig::TAppConfigPtr KikimrConfig;
    NStorage::TStorageConfigPtr StorageConfig;

public:
    TBootstrap(
        const TBootstrapOptionsPtr& options,
        const TString& logComponent,
        const TString& metricsComponent);
    virtual ~TBootstrap();

    void Init();

    void Start();
    void Stop();

    TProgramShouldContinue& GetProgramShouldContinue();

protected:
    virtual void StartComponents() = 0;
    virtual void StopComponents() = 0;
    virtual void InitLWTrace() = 0;

    virtual NServer::IServerPtr CreateServer() = 0;
    virtual TVector<IIncompleteRequestProviderPtr> GetIncompleteRequestProviders() = 0;

private:
    void InitBootstrapLog(const TString& logComponent);
    void InitDiagnosticsConfig();
    void InitDiagnostics();

    void InitKikimrConfigs();
    void InitStorageConfig();
    void InitActorSystem();
};

} // namespace NCloud::NFileStore::NDaemon
