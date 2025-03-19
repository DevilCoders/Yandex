#pragma once

#include "public.h"

#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/events.h>

#include <library/cpp/actors/core/actorid.h>

#include <util/generic/string.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TEvMetering
{
    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TBlockStoreEvents::METERING_START,

        EvWriteMeteringJson,

        EvEnd
    };

    static_assert(
        EvEnd < (int)TBlockStoreEvents::METERING_END,
        "EvEnd expected to be < TBlockStoreEvents::METERING_END");

    //
    // WriteMeteringJson
    //

    struct TEvWriteMeteringJson
        : public NActors::TEventLocal<TEvWriteMeteringJson, EvWriteMeteringJson>
    {
        TString MeteringJson;

        TEvWriteMeteringJson(TString meteringJson)
            : MeteringJson(std::move(meteringJson))
        {}
    };
};

////////////////////////////////////////////////////////////////////////////////

NActors::TActorId MakeMeteringWriterId();

void SendMeteringJson(const NActors::TActorContext& ctx, TString message);

}   // namespace NCloud::NBlockStore::NStorage
