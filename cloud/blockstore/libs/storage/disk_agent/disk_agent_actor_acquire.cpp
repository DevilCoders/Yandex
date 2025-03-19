#include "disk_agent_actor.h"

#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <util/string/join.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NThreading;

////////////////////////////////////////////////////////////////////////////////

void TDiskAgentActor::HandleAcquireDevices(
    const TEvDiskAgent::TEvAcquireDevicesRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_AGENT_COUNTER(AcquireDevices);

    auto* actorSystem = ctx.ActorSystem();
    auto replyFrom = ctx.SelfID;

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        ev->Get()->CallContext,
        std::move(ev->TraceId)
    );

    auto reply = [=] (auto error) {
        auto response = std::make_unique<TEvDiskAgent::TEvAcquireDevicesResponse>(
            std::move(error));

        actorSystem->Send(
            new IEventHandle(
                requestInfo->Sender,
                replyFrom,
                response.release(),
                0,          // flags
                requestInfo->Cookie,
                nullptr,    // forwardOnNondelivery
                std::move(requestInfo->TraceId)));
    };

    const auto* msg = ev->Get();
    const auto& record = msg->Record;
    const TString sessionId = record.GetSessionId();

    TVector<TString> uuids {
        record.GetDeviceUUIDs().begin(),
        record.GetDeviceUUIDs().end()
    };

    try {
        LOG_DEBUG_S(
            ctx,
            TBlockStoreComponents::DISK_AGENT,
            "AcquireDevices: sessionId=" << sessionId
                << ", uuids=" << JoinSeq(",", uuids)
        );

        State->AcquireDevices(
            uuids,
            sessionId,
            ctx.Now(),
            record.GetAccessMode(),
            record.GetMountSeqNumber()
        );

        if (!Spdk || !record.HasRateLimits()) {
            reply(NProto::TError());
            return;
        }

        const NSpdk::TDeviceRateLimits limits {
            record.GetRateLimits().GetIopsLimit(),
            record.GetRateLimits().GetBandwidthLimit(),
            record.GetRateLimits().GetReadBandwidthLimit(),
            record.GetRateLimits().GetWriteBandwidthLimit()
        };

        TVector<TFuture<void>> futures(Reserve(uuids.size()));

        for (const auto& uuid: uuids) {
            const auto& deviceName = State->GetDeviceName(uuid);

            futures.push_back(Spdk->SetRateLimits(deviceName, limits));
        }

        WaitExceptionOrAll(futures).Subscribe(
            [=, uuids = std::move(uuids)] (const auto& future) {
                try {
                    future.GetValue();
                    reply(NProto::TError());
                } catch (...) {
                    State->ReleaseDevices(uuids, sessionId);
                    reply(MakeError(E_FAIL, CurrentExceptionMessage()));
                }
            });
    } catch (const TServiceError& e) {
        LOG_ERROR_S(
            ctx,
            TBlockStoreComponents::DISK_AGENT,
            "AcquireDevices failed: " << e.what()
                << ", uuids=" << JoinSeq(",", uuids)
        );

        reply(MakeError(e.GetCode(), e.what()));
    }
}

}   // namespace NCloud::NBlockStore::NStorage
