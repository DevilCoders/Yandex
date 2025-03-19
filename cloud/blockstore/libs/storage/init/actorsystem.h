#pragma once

#include "public.h"

#include <cloud/blockstore/libs/common/public.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/discovery/public.h>
#include <cloud/blockstore/libs/kikimr/public.h>
#include <cloud/blockstore/libs/logbroker/public.h>
#include <cloud/blockstore/libs/notify/public.h>
#include <cloud/blockstore/libs/rdma/public.h>
#include <cloud/blockstore/libs/service/public.h>
#include <cloud/blockstore/libs/spdk/public.h>
#include <cloud/blockstore/libs/storage/core/public.h>
#include <cloud/blockstore/libs/storage/disk_agent/public.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/public.h>
#include <cloud/blockstore/libs/storage/ss_proxy/public.h>
#include <cloud/blockstore/libs/ydbstats/public.h>

#include <library/cpp/actors/core/defs.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TActorSystemArgs
{
    ui32 NodeId;
    NActors::TScopeId ScopeId;
    NKikimrConfig::TAppConfigPtr AppConfig;

    TDiagnosticsConfigPtr DiagnosticsConfig;
    TStorageConfigPtr StorageConfig;
    TDiskAgentConfigPtr DiskAgentConfig;
    TDiskRegistryProxyConfigPtr DiskRegistryProxyConfig;

    ILoggingServicePtr Logging;
    IAsyncLoggerPtr AsyncLogger;
    IStatsAggregatorPtr StatsAggregator;
    NYdbStats::IYdbVolumesStatsUploaderPtr StatsUploader;
    NDiscovery::IDiscoveryServicePtr DiscoveryService;
    NSpdk::ISpdkEnvPtr Spdk;
    ICachingAllocatorPtr Allocator;
    IFileIOServicePtr FileIOService;
    IStorageProviderPtr AioStorageProvider;
    IProfileLogPtr ProfileLog;
    IBlockDigestGeneratorPtr BlockDigestGenerator;
    ITraceSerializerPtr TraceSerializer;
    NLogbroker::IServicePtr LogbrokerService;
    NNotify::IServicePtr NotifyService;
    IVolumeStatsPtr VolumeStats;
    NRdma::IServerPtr RdmaServer;
    NRdma::IClientPtr RdmaClient;
    ICgroupStatsFetcherPtr CgroupStatsFetcher;
};

////////////////////////////////////////////////////////////////////////////////

IActorSystemPtr CreateActorSystem(const TActorSystemArgs& args);

}   // namespace NCloud::NBlockStore::NStorage
