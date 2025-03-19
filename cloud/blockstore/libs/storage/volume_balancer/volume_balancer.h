#pragma once

#include "public.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/discovery/public.h>
#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/kikimr/public.h>
#include <cloud/blockstore/libs/storage/core/public.h>
#include <cloud/blockstore/libs/ydbstats/public.h>

#include <util/generic/maybe.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

NActors::IActorPtr CreateVolumeBalancerActor(
    NKikimrConfig::TAppConfigPtr appConfig,
    TStorageConfigPtr storageConfig,
    IVolumeStatsPtr volumeStats,
    ICgroupStatsFetcherPtr cgroupStatFetcher,
    NActors::TActorId serviceActorId);

NActors::IActorPtr CreateVolumeBalancerActorStub();

}   // namespace NCloud::NBlockStore::NStorage
