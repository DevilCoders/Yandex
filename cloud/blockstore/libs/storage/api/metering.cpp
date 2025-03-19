#include "metering.h"

#include <cloud/blockstore/libs/kikimr/helpers.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

TActorId MakeMeteringWriterId()
{
    return TActorId(0, "blk-metering");
}

void SendMeteringJson(const TActorContext& ctx, TString message)
{
    auto request = std::make_unique<TEvMetering::TEvWriteMeteringJson>(std::move(message));
    NCloud::Send(ctx, MakeMeteringWriterId(), std::move(request));
}

}    // namespace NCloud::NBlockStore::NStorage
