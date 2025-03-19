#include "metering.h"

#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/storage/api/metering.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TMeteringWriteActor final
    : public TActor<TMeteringWriteActor>
{
private:
    const std::unique_ptr<TLogBackend> MeteringFile;

public:
    TMeteringWriteActor(std::unique_ptr<TLogBackend> meteringFile)
        : TActor(&TThis::StateWork)
        , MeteringFile(std::move(meteringFile))
    {
    }

private:
    STFUNC(StateWork);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void HandleWriteMeteringJson(
        const TEvMetering::TEvWriteMeteringJson::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

void TMeteringWriteActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    Die(ctx);
}

STFUNC(TMeteringWriteActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvMetering::TEvWriteMeteringJson, HandleWriteMeteringJson);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::METERING);
            break;
    }
}

void TMeteringWriteActor::HandleWriteMeteringJson(
    const TEvMetering::TEvWriteMeteringJson::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    try {
        MeteringFile->WriteData(
            TLogRecord(
                ELogPriority::TLOG_INFO,
                msg->MeteringJson.data(),
                msg->MeteringJson.length()));
    } catch (const TFileError& e) {
        LOG_ERROR(ctx, TBlockStoreComponents::METERING,
            "Unable to write metering data (error: %s)",
            e.what());
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateMeteringWriter(std::unique_ptr<TLogBackend> meteringFile)
{
    return std::make_unique<TMeteringWriteActor>(std::move(meteringFile));
}

}    // namespace NCloud::NBlockStore::NStorage
