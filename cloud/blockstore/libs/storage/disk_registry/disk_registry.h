#pragma once

#include "public.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/kikimr/public.h>
#include <cloud/blockstore/libs/logbroker/public.h>
#include <cloud/blockstore/libs/notify/public.h>
#include <cloud/blockstore/libs/storage/core/public.h>

#include <library/cpp/actors/core/actorid.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

NActors::IActorPtr CreateDiskRegistry(
    const NActors::TActorId& owner,
    NKikimr::TTabletStorageInfoPtr storage,
    TStorageConfigPtr config,
    TDiagnosticsConfigPtr diagnosticsConfig,
    NLogbroker::IServicePtr logbrokerService,
    NNotify::IServicePtr notifyService);

}   // namespace NCloud::NBlockStore::NStorage
