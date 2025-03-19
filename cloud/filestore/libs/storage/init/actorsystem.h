#pragma once

#include "public.h"

#include <cloud/filestore/libs/diagnostics/public.h>
#include <cloud/filestore/libs/storage/core/public.h>

#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/diagnostics/public.h>
#include <cloud/storage/core/libs/kikimr/public.h>

#include <library/cpp/actors/core/defs.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TActorSystemArgs
{
    ui32 NodeId;
    NActors::TScopeId ScopeId;
    NKikimrConfig::TAppConfigPtr AppConfig;

    TStorageConfigPtr StorageConfig;
    IAsyncLoggerPtr AsyncLogger;
    IProfileLogPtr ProfileLog;
};

////////////////////////////////////////////////////////////////////////////////

IActorSystemPtr CreateActorSystem(const TActorSystemArgs& args);

}   // namespace NCloud::NFileStore::NStorage
