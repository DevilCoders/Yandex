#pragma once

#include "public.h"

#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/kikimr/public.h>

namespace NCloud::NStorage {

////////////////////////////////////////////////////////////////////////////////

NActors::IActorPtr CreateHiveProxy(
    THiveProxyConfig config,
    IFileIOServicePtr fileIO);

}   // namespace NCloud::NStorage
