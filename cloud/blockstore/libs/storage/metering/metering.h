#pragma once

#include "public.h"

#include <cloud/blockstore/libs/kikimr/public.h>

#include <library/cpp/logger/backend.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

NActors::IActorPtr CreateMeteringWriter(
    std::unique_ptr<TLogBackend> meteringFile);

}   // namespace NCloud::NBlockStore::NStorage
