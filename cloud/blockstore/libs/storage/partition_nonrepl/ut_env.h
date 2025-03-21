#pragma once

#include "config.h"
#include "part_nonrepl_events_private.h"

#include <cloud/blockstore/libs/common/block_range.h>
#include <cloud/blockstore/libs/common/sglist_test.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/stats_service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/core/config.h>

#include <cloud/storage/core/libs/kikimr/helpers.h>

#include <ydb/core/testlib/basics/runtime.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

const auto WaitTimeout = TDuration::Seconds(5);
const ui64 DefaultDeviceBlockSize = 512;

////////////////////////////////////////////////////////////////////////////////

struct TStorageStatsServiceState
    : TAtomicRefCount<TStorageStatsServiceState>
{
    TPartitionDiskCounters Counters{EPublishingPolicy::NonRepl};
};

using TStorageStatsServiceStatePtr = TIntrusivePtr<TStorageStatsServiceState>;

////////////////////////////////////////////////////////////////////////////////

class TStorageStatsServiceMock final
    : public NActors::TActor<TStorageStatsServiceMock>
{
private:
    TStorageStatsServiceStatePtr State;

public:
    TStorageStatsServiceMock(TStorageStatsServiceStatePtr state)
        : TActor(&TThis::StateWork)
        , State(std::move(state))
    {
    }

private:
    STFUNC(StateWork)
    {
        switch (ev->GetTypeRewrite()) {
            HFunc(NActors::TEvents::TEvPoisonPill, HandlePoisonPill);

            HFunc(TEvStatsService::TEvVolumePartCounters, HandleVolumePartCounters);

            default:
                Y_FAIL("Unexpected event %x", ev->GetTypeRewrite());
        }
    }

    void HandlePoisonPill(
        const NActors::TEvents::TEvPoisonPill::TPtr& ev,
        const NActors::TActorContext& ctx)
    {
        Y_UNUSED(ev);

        Die(ctx);
    }

    void HandleVolumePartCounters(
        const TEvStatsService::TEvVolumePartCounters::TPtr& ev,
        const NActors::TActorContext& ctx)
    {
        Y_UNUSED(ctx);

        State->Counters = *ev->Get()->DiskCounters;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TDummyActor final
    : public NActors::TActor<TDummyActor>
{
public:
    TDummyActor()
        : TActor(&TThis::StateWork)
    {
    }

private:
    STFUNC(StateWork)
    {
        switch (ev->GetTypeRewrite()) {
            HFunc(NActors::TEvents::TEvPoisonPill, HandlePoisonPill);

            HFunc(TEvVolume::TEvNonreplicatedPartitionCounters, HandleVolumePartCounters);

            HFunc(TEvVolume::TEvRdmaUnavailable, HandleRdmaUnavailable);

            HFunc(TEvDiskRegistry::TEvFinishMigrationRequest, HandleFinishMigration);

            default:
                Y_FAIL("Unexpected event %x", ev->GetTypeRewrite());
        }
    }

    void HandlePoisonPill(
        const NActors::TEvents::TEvPoisonPill::TPtr& ev,
        const NActors::TActorContext& ctx)
    {
        Y_UNUSED(ev);

        Die(ctx);
    }

    void HandleVolumePartCounters(
        const TEvVolume::TEvNonreplicatedPartitionCounters::TPtr& ev,
        const NActors::TActorContext& ctx)
    {
        auto event = std::make_unique<NActors::IEventHandle>(
            MakeStorageStatsServiceId(),
            ev->Sender,
            new TEvStatsService::TEvVolumePartCounters(
                "", // diskId
                std::move(ev->Get()->DiskCounters),
                0,
                0,
                false,
                NBlobMetrics::TBlobLoadMetrics()),
            ev->Flags,
            ev->Cookie,
            nullptr,
            std::move(ev->TraceId));
        ctx.Send(event.release());
    }

    void HandleRdmaUnavailable(
        const TEvVolume::TEvRdmaUnavailable::TPtr& ev,
        const NActors::TActorContext& ctx)
    {
        Y_UNUSED(ev);
        Y_UNUSED(ctx);
    }

    void HandleFinishMigration(
        const TEvDiskRegistry::TEvFinishMigrationRequest::TPtr& ev,
        const NActors::TActorContext& ctx)
    {
        NCloud::Reply(ctx, *ev,
            std::make_unique<TEvDiskRegistry::TEvFinishMigrationResponse>());
    }
};

////////////////////////////////////////////////////////////////////////////////

class TPartitionClient
{
private:
    NActors::TTestActorRuntime& Runtime;
    ui32 NodeIdx;
    NActors::TActorId ActorId;
    NActors::TActorId Sender;

public:
    TPartitionClient(
            NActors::TTestActorRuntime& runtime,
            NActors::TActorId actorId)
        : Runtime(runtime)
        , NodeIdx(0)
        , ActorId(actorId)
        , Sender(runtime.AllocateEdgeActor())
    {}

    const NActors::TActorId& GetActorId() const
    {
        return ActorId;
    }

    template <typename TRequest>
    void SendRequest(
        const NActors::TActorId& recipient,
        std::unique_ptr<TRequest> request)
    {
        auto* ev = new NActors::IEventHandle(
            recipient,
            Sender,
            request.release(),
            0,          // flags
            0,          // cookie
            nullptr,    // forwardOnNondelivery
            NWilson::TTraceId::NewTraceId());

        Runtime.Send(ev, NodeIdx);
    }

    template <typename TResponse>
    std::unique_ptr<TResponse> RecvResponse()
    {
        TAutoPtr<NActors::IEventHandle> handle;
        Runtime.GrabEdgeEventRethrow<TResponse>(handle, WaitTimeout);

        UNIT_ASSERT(handle);
        return std::unique_ptr<TResponse>(handle->Release<TResponse>().Release());
    }

    auto CreateReadBlocksRequest(const TBlockRange64& blockRange)
    {
        auto request = std::make_unique<TEvService::TEvReadBlocksRequest>();
        request->Record.SetStartIndex(blockRange.Start);
        request->Record.SetBlocksCount(blockRange.Size());

        return request;
    }

    auto CreateWriteBlocksRequest(
        const TBlockRange64& blockRange,
        char fill,
        ui32 blocksInBuffer = 1)
    {
        auto request = std::make_unique<TEvService::TEvWriteBlocksRequest>();

        request->Record.SetStartIndex(blockRange.Start);
        for (ui32 i = 0; i < blockRange.Size(); i += blocksInBuffer) {
            auto& b = *request->Record.MutableBlocks()->AddBuffers();
            b.resize(Min<ui32>(
                blocksInBuffer,
                (blockRange.Size() - i)
            ) * DefaultBlockSize, fill);
        }

        return request;
    }

    auto CreateZeroBlocksRequest(const TBlockRange64& blockRange)
    {
        auto request = std::make_unique<TEvService::TEvZeroBlocksRequest>();
        request->Record.SetStartIndex(blockRange.Start);
        request->Record.SetBlocksCount(blockRange.Size());

        return request;
    }

    auto CreateWriteBlocksLocalRequest(TBlockRange64 range, const TString& content)
    {
        TSgList sglist(range.Size(), TBlockDataRef{ content.data(), content.size() });

        auto request = std::make_unique<TEvService::TEvWriteBlocksLocalRequest>();
        request->Record.Sglist = TGuardedSgList(std::move(sglist));
        request->Record.SetStartIndex(range.Start);
        request->Record.BlocksCount = range.Size();
        request->Record.BlockSize = content.size() / range.Size();

        return request;
    }

    auto CreateReadBlocksLocalRequest(TBlockRange64 range, TGuardedSgList sglist)
    {
        auto request = std::make_unique<TEvService::TEvReadBlocksLocalRequest>();
        request->Record.Sglist = std::move(sglist);
        request->Record.SetStartIndex(range.Start);
        request->Record.SetBlocksCount(range.Size());
        request->Record.BlockSize = DefaultBlockSize;

        return request;
    }


#define BLOCKSTORE_DECLARE_METHOD(name, ns)                                    \
    template <typename... Args>                                                \
    void Send##name##Request(Args&&... args)                                   \
    {                                                                          \
        auto request = Create##name##Request(std::forward<Args>(args)...);     \
        SendRequest(ActorId, std::move(request));                              \
    }                                                                          \
                                                                               \
    std::unique_ptr<ns::TEv##name##Response> Recv##name##Response()            \
    {                                                                          \
        return RecvResponse<ns::TEv##name##Response>();                        \
    }                                                                          \
                                                                               \
    template <typename... Args>                                                \
    std::unique_ptr<ns::TEv##name##Response> name(Args&&... args)              \
    {                                                                          \
        auto request = Create##name##Request(std::forward<Args>(args)...);     \
        SendRequest(ActorId, std::move(request));                              \
                                                                               \
        auto response = RecvResponse<ns::TEv##name##Response>();               \
        UNIT_ASSERT_C(                                                         \
            SUCCEEDED(response->GetStatus()),                                  \
            response->GetErrorReason());                                       \
        return response;                                                       \
    }                                                                          \
// BLOCKSTORE_DECLARE_METHOD

    BLOCKSTORE_DECLARE_METHOD(ReadBlocks, TEvService);
    BLOCKSTORE_DECLARE_METHOD(WriteBlocks, TEvService);
    BLOCKSTORE_DECLARE_METHOD(ReadBlocksLocal, TEvService);
    BLOCKSTORE_DECLARE_METHOD(WriteBlocksLocal, TEvService);
    BLOCKSTORE_DECLARE_METHOD(ZeroBlocks, TEvService);

#undef BLOCKSTORE_DECLARE_METHOD
};

inline void ToLogicalBlocks(NProto::TDeviceConfig& device)
{
    const auto k = DefaultBlockSize / DefaultDeviceBlockSize;

    device.SetBlocksCount(device.GetBlocksCount() / k);
    device.SetBlockSize(DefaultBlockSize);
}

inline TDevices ToLogicalBlocks(TDevices devices)
{
    for (auto& device: devices) {
        ToLogicalBlocks(device);
    }

    return devices;
}

////////////////////////////////////////////////////////////////////////////////

inline void WaitForMigrations(
    NActors::TTestBasicRuntime& runtime,
    ui32 rangeCount)
{
    ui32 migratedRanges = 0;
    runtime.SetObserverFunc([&] (auto& runtime, auto& event) {
        switch (event->GetTypeRewrite()) {
            case TEvNonreplPartitionPrivate::EvRangeMigrated: {
                auto* msg =
                    event->template Get<TEvNonreplPartitionPrivate::TEvRangeMigrated>();
                if (!HasError(msg->Error)) {
                    ++migratedRanges;
                }
                break;
            }
        }
        return NActors::TTestActorRuntime::DefaultObserverFunc(runtime, event);
    });

    ui32 i = 0;
    while (migratedRanges < rangeCount && i++ < 6) {
        NActors::TDispatchOptions options;
        options.FinalEvents = {
            NActors::TDispatchOptions::TFinalEventCondition(
                TEvNonreplPartitionPrivate::EvRangeMigrated)
        };

        runtime.DispatchEvents(options);
    }

    UNIT_ASSERT_VALUES_EQUAL(rangeCount, migratedRanges);
}

}   // namespace NCloud::NBlockStore::NStorage
