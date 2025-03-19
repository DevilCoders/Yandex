#pragma once

#include "public.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/kikimr/public.h>
#include <cloud/blockstore/libs/rdma/public.h>
#include <cloud/blockstore/libs/service/public.h>
#include <cloud/blockstore/libs/spdk/public.h>
#include <cloud/blockstore/libs/storage/core/public.h>
#include <cloud/blockstore/libs/storage/disk_agent/public.h>
#include <cloud/blockstore/libs/storage/disk_registry_proxy/public.h>
#include <cloud/blockstore/libs/storage/ss_proxy/public.h>

#include <library/cpp/actors/core/defs.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TDiskAgentActorSystemArgs
{
    ui32 NodeId;
    NActors::TScopeId ScopeId;
    NKikimrConfig::TAppConfigPtr AppConfig;

    TStorageConfigPtr StorageConfig;
    TDiskAgentConfigPtr DiskAgentConfig;
    TDiskRegistryProxyConfigPtr DiskRegistryProxyConfig;

    ILoggingServicePtr Logging;
    IAsyncLoggerPtr AsyncLogger;
    NSpdk::ISpdkEnvPtr Spdk;
    ICachingAllocatorPtr Allocator;
    IFileIOServicePtr FileIOService;
    IStorageProviderPtr AioStorageProvider;
    IProfileLogPtr ProfileLog;
    IBlockDigestGeneratorPtr BlockDigestGenerator;
    NRdma::IServerPtr RdmaServer;
};

////////////////////////////////////////////////////////////////////////////////

IActorSystemPtr CreateDiskAgentActorSystem(const TDiskAgentActorSystemArgs& args);

}   // namespace NCloud::NBlockStore::NStorage
