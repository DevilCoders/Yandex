#pragma once

#include <cloud/blockstore/libs/diagnostics/public.h>

#include <cloud/storage/core/libs/kikimr/public.h>


namespace NCloud::NBlockStore::NStorage::NUserStats {

////////////////////////////////////////////////////////////////////////////////

NActors::IActorPtr CreateStorageUserStats(IVolumeStatsPtr volumeStats);

}   // NCloud::NBlockStore::NStorage::NUserStats
