#pragma once

#include "private.h"

#include <cloud/blockstore/libs/common/public.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/discovery/public.h>
#include <cloud/blockstore/libs/endpoints/public.h>
#include <cloud/blockstore/libs/endpoints_grpc/public.h>
#include <cloud/blockstore/libs/kikimr/public.h>
#include <cloud/blockstore/libs/logbroker/public.h>
#include <cloud/blockstore/libs/nbd/public.h>
#include <cloud/blockstore/libs/notify/public.h>
#include <cloud/blockstore/libs/nvme/public.h>
#include <cloud/blockstore/libs/rdma/rdma.h>
#include <cloud/blockstore/libs/server/public.h>
#include <cloud/blockstore/libs/service/public.h>
#include <cloud/blockstore/libs/service_local/public.h>
#include <cloud/blockstore/libs/spdk/public.h>
#include <cloud/blockstore/libs/vhost/public.h>
#include <cloud/blockstore/libs/ydbstats/public.h>

#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/coroutine/public.h>

#include <library/cpp/actors/util/should_continue.h>
#include <library/cpp/logger/log.h>

namespace NCloud::NBlockStore::NServer {

////////////////////////////////////////////////////////////////////////////////

struct TConfigInitializer;

////////////////////////////////////////////////////////////////////////////////

class TBootstrap
{
private:
    std::unique_ptr<TConfigInitializer> Configs;

    ILoggingServicePtr BootstrapLogging;
    TLog Log;

    ILoggingServicePtr Logging;
    TLog GrpcLog;
    TLog SpdkLog;

    ITimerPtr Timer;
    ISchedulerPtr Scheduler;
    ITaskQueuePtr BackgroundThreadPool;
    ISchedulerPtr BackgroundScheduler;
    ITaskQueuePtr SubmissionQueue;
    ITaskQueuePtr CompletionQueue;
    IActorSystemPtr ActorSystem;
    IAsyncLoggerPtr AsyncLogger;
    IMonitoringServicePtr Monitoring;
    IRequestStatsPtr RequestStats;
    IVolumeStatsPtr VolumeStats;
    IServerStatsPtr ServerStats;
    IStatsUpdaterPtr ServerStatsUpdater;
    IStatsAggregatorPtr StatsAggregator;
    IClientPercentileCalculatorPtr ClientPercentiles;
    NYdbStats::IYdbVolumesStatsUploaderPtr StatsUploader;
    NYdbStats::IYdbStoragePtr YdbStorage;
    TVector<ITraceReaderPtr> TraceReaders;
    ITraceProcessorPtr TraceProcessor;
    ITraceSerializerPtr TraceSerializer;
    NDiscovery::IDiscoveryServicePtr DiscoveryService;
    IProfileLogPtr ProfileLog;
    IBlockDigestGeneratorPtr BlockDigestGenerator;
    IBlockStorePtr Service;
    ISocketEndpointListenerPtr GrpcEndpointListener;
    NVhost::IServerPtr VhostServer;
    NBD::IServerPtr NbdServer;
    IFileIOServicePtr FileIOService;
    IStorageProviderPtr StorageProvider;
    TExecutorPtr Executor;
    IServerPtr Server;
    NSpdk::ISpdkEnvPtr Spdk;
    ICachingAllocatorPtr Allocator;
    IStorageProviderPtr AioStorageProvider;
    IEndpointServicePtr EndpointService;
    NLogbroker::IServicePtr LogbrokerService;
    NNotify::IServicePtr NotifyService;
    NRdma::TRdmaHandler Rdma;
    NRdma::IServerPtr RdmaServer;
    NRdma::IClientPtr RdmaClient;
    ITaskQueuePtr RdmaThreadPool;
    NNvme::INvmeManagerPtr NvmeManager;
    ICgroupStatsFetcherPtr CgroupStatsFetcher;

    TProgramShouldContinue ShouldContinue;
    TVector<TString> PostponedCriticalEvents;

public:
    TBootstrap(TOptionsPtr options);
    ~TBootstrap();

    void Init(IDeviceHandlerFactoryPtr deviceHandlerFactory);

    void Start();
    void Stop();

    TProgramShouldContinue& GetShouldContinue();
    IBlockStorePtr GetBlockStoreService();

private:
    void InitLWTrace();

    void InitConfigs();
    void InitRdmaClient();
    void InitSpdk();
    void InitProfileLog();
    void InitKikimrService();
    void InitLocalService();
    void InitNullService();

    void InitDbgConfigs();
};

}   // namespace NCloud::NBlockStore::NServer
