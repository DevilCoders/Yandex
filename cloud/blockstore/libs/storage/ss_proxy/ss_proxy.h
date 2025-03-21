#pragma once

#include "public.h"

#include <cloud/blockstore/libs/kikimr/public.h>
#include <cloud/blockstore/libs/storage/core/public.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

NActors::IActorPtr CreateSSProxy(
    TStorageConfigPtr config,
    IFileIOServicePtr fileIO);

}   // namespace NCloud::NBlockStore::NStorage
