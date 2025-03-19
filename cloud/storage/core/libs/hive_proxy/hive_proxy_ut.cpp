#include "hive_proxy.h"

#include <cloud/storage/core/libs/aio/service.h>
#include <cloud/storage/core/libs/api/hive_proxy.h>
#include <cloud/storage/core/libs/common/file_io_service.h>

#include <ydb/core/base/blobstorage.h>
#include <ydb/core/base/hive.h>
#include <ydb/core/base/tablet_resolver.h>
#include <ydb/core/mind/local.h>
#include <ydb/core/tablet_flat/tablet_flat_executed.h>
#include <ydb/core/testlib/basics/runtime.h>
#include <ydb/core/testlib/basics/appdata.h>
#include <ydb/core/testlib/basics/helpers.h>
#include <ydb/core/testlib/tablet_helpers.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

static constexpr ui64 FakeHiveTablet = 0x0000000000000001;
static constexpr ui64 FakeSchemeRoot = 0x00000000008401F0;
static constexpr ui64 FakeTablet2 = 0x0000000000840102;
static constexpr ui64 FakeTablet3 = 0x0000000000840103;
static constexpr ui64 FakeMissingTablet = 0x0000000000840104;

////////////////////////////////////////////////////////////////////////////////

struct THiveMockState
    : public TAtomicRefCount<THiveMockState>
{
    using TPtr = TIntrusivePtr<THiveMockState>;

    THashMap<ui64, TActorId> LockedTablets;
    THashMap<ui64, ui64> ReconnectRequests;
    THashMap<ui64, TTabletStorageInfoPtr> StorageInfos;
    THashMap<ui64, ui32> KnownGenerations;
    THashSet<ui32> DrainableNodeIds;
    THashSet<ui32> DownNodeIds;
};

////////////////////////////////////////////////////////////////////////////////

class THiveMock final
    : public TActor<THiveMock>
    , public NTabletFlatExecutor::TTabletExecutedFlat
{
private:
    THiveMockState::TPtr State;

public:
    THiveMock(
            const TActorId& owner,
            TTabletStorageInfo* info,
            THiveMockState::TPtr state)
        : TActor(&TThis::StateInit)
        , TTabletExecutedFlat(info, owner, nullptr)
        , State(std::move(state))
    {
        UNIT_ASSERT(State);
    }

private:
    void OnActivateExecutor(const TActorContext& ctx) override
    {
        Y_UNUSED(ctx);
        Become(&TThis::StateWork);
    }

    void OnDetach(const TActorContext& ctx) override
    {
        Die(ctx);
    }

    void OnTabletDead(
        TEvTablet::TEvTabletDead::TPtr& ev,
        const TActorContext& ctx) override
    {
        Y_UNUSED(ev);
        Die(ctx);
    }

    void Enqueue(STFUNC_SIG) override
    {
        Y_UNUSED(ctx);
        Y_FAIL("Unexpected event %x", ev->GetTypeRewrite());
    }

    STFUNC(StateInit)
    {
        StateInitImpl(ev, ctx);
    }

    STFUNC(StateWork)
    {
        switch (ev->GetTypeRewrite()) {
            HFunc(TEvents::TEvPoisonPill, HandlePoison);
            HFunc(TEvHive::TEvLockTabletExecution, HandleLock);
            HFunc(TEvHive::TEvUnlockTabletExecution, HandleUnlock);
            HFunc(TEvHive::TEvGetTabletStorageInfo, HandleInfo);
            HFunc(TEvHive::TEvInitiateTabletExternalBoot, HandleBoot);
            HFunc(TEvHive::TEvReassignTablet, HandleReassignTablet);
            HFunc(TEvHive::TEvDrainNode, HandleDrainNode);
            IgnoreFunc(TEvTabletPipe::TEvServerConnected);
            IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);
            default:
                if (!HandleDefaultEvents(ev, ctx)) {
                    Y_FAIL("Unexpected event %x", ev->GetTypeRewrite());
                }
        }
    }

    void HandlePoison(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx)
    {
        Y_UNUSED(ev);
        ctx.Send(Tablet(), new TEvents::TEvPoisonPill);
    }

    TActorId GetLockOwner(ui64 tabletId)
    {
        auto it = State->LockedTablets.find(tabletId);
        if (it != State->LockedTablets.end()) {
            return it->second;
        }
        return TActorId();
    }

    void SetLockOwner(
        const TActorContext& ctx,
        ui64 tabletId,
        TActorId newOwner)
    {
        auto prev = State->LockedTablets[tabletId];
        State->LockedTablets[tabletId] = newOwner;
        if (prev && prev != newOwner) {
            ctx.Send(prev, new TEvHive::TEvLockTabletExecutionLost(tabletId));
        }
    }

    TTabletStorageInfoPtr GetStorageInfo(ui64 tabletId)
    {
        auto it = State->StorageInfos.find(tabletId);
        if (it != State->StorageInfos.end()) {
            return it->second;
        }
        return nullptr;
    }

    void HandleLock(
        const TEvHive::TEvLockTabletExecution::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto* msg = ev->Get();

        UNIT_ASSERT_C(
            !msg->Record.HasOwnerActor(),
            "owner actors not supported");

        ui64 tabletId = msg->Record.GetTabletID();
        if (tabletId == FakeMissingTablet) {
            ctx.Send(
                ev->Sender,
                new TEvHive::TEvLockTabletExecutionResult(
                    tabletId,
                    NKikimrProto::ERROR,
                    "Tablet not found"));
            return;
        }

        if (msg->Record.GetReconnect()) {
            ++State->ReconnectRequests[tabletId];

            if (GetLockOwner(tabletId) != ev->Sender) {
                ctx.Send(
                    ev->Sender,
                    new TEvHive::TEvLockTabletExecutionResult(
                        tabletId,
                        NKikimrProto::ERROR,
                        "Not locked"));
                return;
            }
        } else {
            SetLockOwner(ctx, tabletId, ev->Sender);
        }

        ctx.Send(
            ev->Sender,
            new TEvHive::TEvLockTabletExecutionResult(
                tabletId,
                NKikimrProto::OK,
                ""));
    }

    void HandleUnlock(
        const TEvHive::TEvUnlockTabletExecution::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto* msg = ev->Get();

        UNIT_ASSERT_C(
            !msg->Record.HasOwnerActor(),
            "owner actors not supported");

        ui64 tabletId = msg->Record.GetTabletID();

        if (GetLockOwner(tabletId) != ev->Sender) {
            ctx.Send(
                ev->Sender,
                new TEvHive::TEvUnlockTabletExecutionResult(
                    tabletId,
                    NKikimrProto::ERROR,
                    "Not locked"));
            return;
        }

        State->LockedTablets.erase(tabletId);
        ctx.Send(
            ev->Sender,
            new TEvHive::TEvUnlockTabletExecutionResult(
                tabletId,
                NKikimrProto::OK,
                ""));
    }

    void HandleInfo(
        const TEvHive::TEvGetTabletStorageInfo::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto* msg = ev->Get();

        ui64 tabletId = msg->Record.GetTabletID();
        auto info = GetStorageInfo(tabletId);
        if (!info) {
            ctx.Send(
                ev->Sender,
                new TEvHive::TEvGetTabletStorageInfoResult(
                    tabletId,
                    NKikimrProto::ERROR));
            return;
        }

        ctx.Send(
            ev->Sender,
            new TEvHive::TEvGetTabletStorageInfoRegistered(tabletId));

        ctx.Send(
            ev->Sender,
            new TEvHive::TEvGetTabletStorageInfoResult(tabletId, *info));
    }

    void HandleBoot(
        const TEvHive::TEvInitiateTabletExternalBoot::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto& msg = ev->Get();

        ui64 tabletId = msg->Record.GetTabletID();
        auto info = GetStorageInfo(tabletId);
        if (!info) {
            ctx.Send(
                ev->Sender,
                new TEvHive::TEvBootTabletReply(NKikimrProto::ERROR));
            return;
        }

        ctx.Send(ev->Sender,
            new TEvLocal::TEvBootTablet(
                *info,
                0,
                ++State->KnownGenerations[tabletId]));
    }

    void HandleReassignTablet(
        const TEvHive::TEvReassignTablet::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto& msg = ev->Get();

        ui64 tabletId = msg->Record.GetTabletID();
        auto info = GetStorageInfo(tabletId);

        if (!info) {
            ctx.Send(
                ev->Sender,
                new TEvHive::TEvBootTabletReply(NKikimrProto::ERROR));
            return;
        }

        auto gen = ++State->KnownGenerations[tabletId];

        UNIT_ASSERT(info->Channels.size());

        for (auto channel: msg->Record.GetChannels()) {
            UNIT_ASSERT(channel < info->Channels.size());
            auto& channelInfo = info->Channels[channel];
            channelInfo.History.emplace_back(
                gen,
                channelInfo.History.back().GroupID + 1
            );
        }

        ctx.Send(ev->Sender, new TEvHive::TEvTabletCreationResult());
    }

    void HandleDrainNode(
        const TEvHive::TEvDrainNode::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto& msg = ev->Get();

        NKikimrProto::EReplyStatus replyStatus = NKikimrProto::OK;
        ui64 nodeId = msg->Record.GetNodeID();
        if (State->DrainableNodeIds.contains(nodeId)) {
            if (msg->Record.GetKeepDown()) {
                State->DownNodeIds.insert(nodeId);
            }
        } else {
            replyStatus = NKikimrProto::ERROR;
        }

        ctx.Send(ev->Sender, new TEvHive::TEvDrainNodeResult(replyStatus));
    }
};

////////////////////////////////////////////////////////////////////////////////

void BootHiveMock(
    TTestActorRuntime& runtime,
    ui64 tabletId,
    THiveMockState::TPtr state)
{
    TActorId actorId = CreateTestBootstrapper(
        runtime,
        CreateTestTabletInfo(tabletId, TTabletTypes::Hive),
        [=] (const TActorId& owner, TTabletStorageInfo* info) -> IActor* {
            return new THiveMock(owner, info, state);
        });
    runtime.EnableScheduleForActor(actorId);

    {
        TDispatchOptions options;
        options.FinalEvents.push_back(
            TDispatchOptions::TFinalEventCondition(TEvTablet::EvBoot, 1));
        runtime.DispatchEvents(options);
    }
}

////////////////////////////////////////////////////////////////////////////////

struct TTestEnv
{
    TTestActorRuntime& Runtime;
    bool Debug;
    THiveMockState::TPtr HiveState;
    IFileIOServicePtr FileIOService;
    TActorId HiveProxyActorId;

    TTestEnv(
            TTestActorRuntime& runtime,
            TString tabletBootInfoCacheFilePath = "",
            bool fallbackMode = false,
            bool debug = false)
        : Runtime(runtime)
        , Debug(debug)
        , HiveState(MakeIntrusive<THiveMockState>())
        , FileIOService(CreateAIOService())
    {
        FileIOService->Start();

        TAppPrepare app;

        AddDomain(app, 0, 0, FakeHiveTablet, FakeSchemeRoot);

        SetupLogging();
        SetupChannelProfiles(app);
        SetupTabletServices(Runtime, &app, true);

        BootHiveMock(Runtime, FakeHiveTablet, HiveState);

        SetupHiveProxy(tabletBootInfoCacheFilePath, fallbackMode);
    }

    ~TTestEnv()
    {
        FileIOService->Stop();
    }

    void AddDomain(TAppPrepare& app, ui32 domainUid, ui32 ssId, ui64 hive, ui64 schemeRoot)
    {
        ui32 planResolution = 50;
        app.ClearDomainsAndHive();
        app.AddDomain(TDomainsInfo::TDomain::ConstructDomainWithExplicitTabletIds(
            "MyRoot", domainUid, schemeRoot,
            ssId, ssId, TVector<ui32>{ssId},
            domainUid, TVector<ui32>{domainUid},
            planResolution,
            TVector<ui64>{TDomainsInfo::MakeTxCoordinatorIDFixed(domainUid, 1)},
            TVector<ui64>{},
            TVector<ui64>{},
            DefaultPoolKinds()).Release());
        app.AddHive(domainUid, hive);
    }

    void SetupLogging()
    {
        if (Debug) {
            Runtime.SetLogPriority(NLog::InvalidComponent, NLog::PRI_DEBUG);
        }
        Runtime.SetLogPriority(NKikimrServices::BS_NODE, NLog::PRI_ERROR);
    }

    void SetupHiveProxy(TString tabletBootInfoCacheFilePath, bool fallbackMode)
    {
        THiveProxyConfig config{
            .PipeClientRetryCount = 4,
            .PipeClientMinRetryTime = TDuration::Seconds(1),
            .HiveLockExpireTimeout = TDuration::Seconds(30),
            .LogComponent = 0,
            .TabletBootInfoCacheFilePath = tabletBootInfoCacheFilePath,
            .FallbackMode = fallbackMode,
        };
        HiveProxyActorId = Runtime.Register(
            CreateHiveProxy(std::move(config), FileIOService).release());
        Runtime.EnableScheduleForActor(HiveProxyActorId);
        Runtime.RegisterService(MakeHiveProxyServiceId(), HiveProxyActorId);
    }

    void EnableTabletResolverScheduling(ui32 nodeIdx = 0)
    {
        auto actorId = Runtime.GetLocalServiceId(
            MakeTabletResolverID(),
            nodeIdx);
        UNIT_ASSERT(actorId);
        Runtime.EnableScheduleForActor(actorId);
    }

    void RebootHive()
    {
        auto sender = Runtime.AllocateEdgeActor();
        RebootTablet(Runtime, FakeHiveTablet, sender);
    }

    void SendLockRequest(
        const TActorId& sender,
        ui64 tabletId,
        ui32 errorCode = 0)
    {
        Runtime.Send(new IEventHandle(
            MakeHiveProxyServiceId(),
            sender,
            new TEvHiveProxy::TEvLockTabletRequest(tabletId)));
        TAutoPtr<IEventHandle> handle;
        auto* event =
            Runtime.GrabEdgeEvent<TEvHiveProxy::TEvLockTabletResponse>(handle);
        UNIT_ASSERT(event);
        UNIT_ASSERT_VALUES_EQUAL(event->GetError().GetCode(), errorCode);
    }

    void SendUnlockRequest(
        const TActorId& sender,
        ui64 tabletId,
        ui32 errorCode = 0)
    {
        Runtime.Send(new IEventHandle(
            MakeHiveProxyServiceId(),
            sender,
            new TEvHiveProxy::TEvUnlockTabletRequest(tabletId)));
        TAutoPtr<IEventHandle> handle;
        auto* event =
            Runtime.GrabEdgeEvent<TEvHiveProxy::TEvUnlockTabletResponse>(handle);
        UNIT_ASSERT(event);
        UNIT_ASSERT_VALUES_EQUAL(event->GetError().GetCode(), errorCode);
    }

    void WaitLockLost(
        const TActorId& sender,
        ui64 tabletId,
        ui32 errorCode = 0)
    {
        TAutoPtr<IEventHandle> handle;
        TEvHiveProxy::TEvTabletLockLost* event = nullptr;
        do {
            event =
                Runtime.GrabEdgeEvent<TEvHiveProxy::TEvTabletLockLost>(handle);
        } while (handle && handle->GetRecipientRewrite() != sender);
        UNIT_ASSERT(event);
        UNIT_ASSERT_VALUES_EQUAL(event->TabletId, tabletId);
        UNIT_ASSERT_VALUES_EQUAL(event->GetError().GetCode(), errorCode);
    }

    TTabletStorageInfoPtr SendGetStorageInfoRequest(
        const TActorId& sender,
        ui64 tabletId,
        ui32 errorCode = 0)
    {
        Runtime.Send(new IEventHandle(
            MakeHiveProxyServiceId(),
            sender,
            new TEvHiveProxy::TEvGetStorageInfoRequest(tabletId)));
        TAutoPtr<IEventHandle> handle;
        auto* event =
            Runtime.GrabEdgeEvent<TEvHiveProxy::TEvGetStorageInfoResponse>(handle);
        UNIT_ASSERT(event);
        UNIT_ASSERT_VALUES_EQUAL(event->GetError().GetCode(), errorCode);
        return event->StorageInfo;
    }

    TEvHiveProxy::TBootExternalResponse SendBootExternalRequest(
        const TActorId& sender,
        ui64 tabletId,
        ui32 errorCode)
    {
        Runtime.Send(new IEventHandle(
            MakeHiveProxyServiceId(),
            sender,
            new TEvHiveProxy::TEvBootExternalRequest(tabletId)));
        auto ev = Runtime.GrabEdgeEvent<TEvHiveProxy::TEvBootExternalResponse>(sender);
        UNIT_ASSERT(ev);
        const auto* msg = ev->Get();
        UNIT_ASSERT_VALUES_EQUAL(msg->GetStatus(), errorCode);
        return *msg;
    }

    TEvHiveProxy::TReassignTabletResponse SendReassignTabletRequest(
        const TActorId& sender,
        ui64 tabletId,
        TVector<ui32> channels,
        ui32 errorCode)
    {
        Runtime.Send(new IEventHandle(
            MakeHiveProxyServiceId(),
            sender,
            new TEvHiveProxy::TEvReassignTabletRequest(
                tabletId,
                std::move(channels)
            )
        ));
        using TResponse = TEvHiveProxy::TEvReassignTabletResponse;
        auto ev = Runtime.GrabEdgeEvent<TResponse>(sender);
        UNIT_ASSERT(ev);
        const auto* msg = ev->Get();
        UNIT_ASSERT_VALUES_EQUAL_C(
            msg->GetStatus(),
            errorCode,
            msg->GetErrorReason()
        );

        return *msg;
    }

    TEvHiveProxy::TDrainNodeResponse SendDrainNodeRequest(
        const TActorId& sender,
        bool keepDown,
        ui32 errorCode)
    {
        Runtime.Send(new IEventHandle(
            MakeHiveProxyServiceId(),
            sender,
            new TEvHiveProxy::TEvDrainNodeRequest(keepDown)
        ));
        using TResponse = TEvHiveProxy::TEvDrainNodeResponse;
        auto ev = Runtime.GrabEdgeEvent<TResponse>(sender);
        UNIT_ASSERT(ev);
        const auto* msg = ev->Get();
        UNIT_ASSERT_VALUES_EQUAL_C(
            msg->GetStatus(),
            errorCode,
            msg->GetErrorReason()
        );

        return *msg;
    }

    TEvHiveProxy::TSyncTabletBootInfoCacheResponse SendSyncTabletBootInfoCache(
        const TActorId& sender,
        ui32 errorCode)
    {
        Runtime.Send(new IEventHandle(
            MakeHiveProxyServiceId(),
            sender,
            new TEvHiveProxy::TEvSyncTabletBootInfoCacheRequest()));
        auto ev =
            Runtime.GrabEdgeEvent<TEvHiveProxy::TEvSyncTabletBootInfoCacheResponse>(
                sender);
        UNIT_ASSERT(ev);
        const auto* msg = ev->Get();
        UNIT_ASSERT_VALUES_EQUAL(msg->GetStatus(), errorCode);
        return *msg;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(THiveProxyTest)
{
    Y_UNIT_TEST(LockUnlock)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        auto sender = runtime.AllocateEdgeActor();

        env.SendLockRequest(sender, FakeTablet2);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet2],
            env.HiveProxyActorId);

        env.SendUnlockRequest(sender, FakeTablet2);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet2],
            TActorId());
    }

    Y_UNIT_TEST(LockSameTabletTwice)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        auto sender = runtime.AllocateEdgeActor();

        env.SendLockRequest(sender, FakeTablet2);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet2],
            env.HiveProxyActorId);

        env.SendLockRequest(sender, FakeTablet2, E_REJECTED);
    }

    Y_UNIT_TEST(LockMissingTablet) {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        auto sender = runtime.AllocateEdgeActor();

        env.SendLockRequest(sender, FakeMissingTablet, MAKE_KIKIMR_ERROR(NKikimrProto::ERROR));
    }

    Y_UNIT_TEST(LockDifferentTablets)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        auto sender = runtime.AllocateEdgeActor();

        env.SendLockRequest(sender, FakeTablet2);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet2],
            env.HiveProxyActorId);

        env.SendLockRequest(sender, FakeTablet3);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet3],
            env.HiveProxyActorId);
    }

    Y_UNIT_TEST(LockStolenNotification)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        auto sender = runtime.AllocateEdgeActor();

        env.SendLockRequest(sender, FakeTablet2);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet2],
            env.HiveProxyActorId);

        {
            auto senderB = runtime.AllocateEdgeActor();
            runtime.SendToPipe(
                FakeHiveTablet,
                senderB,
                new TEvHive::TEvLockTabletExecution(FakeTablet2));

            TAutoPtr<IEventHandle> handle;
            auto response = runtime.GrabEdgeEvent<TEvHive::TEvLockTabletExecutionResult>(handle);

            UNIT_ASSERT(response);
            UNIT_ASSERT_VALUES_EQUAL(handle->GetRecipientRewrite(), senderB);
            UNIT_ASSERT_VALUES_EQUAL(
                response->Record.GetStatus(),
                NKikimrProto::OK);
            UNIT_ASSERT_VALUES_EQUAL(
                env.HiveState->LockedTablets[FakeTablet2],
                senderB);
        }

        // Virtual lock owner should receive a lost notification
        env.WaitLockLost(sender, FakeTablet2);

        // New lock request should succeed, stealing the lock back
        env.SendLockRequest(sender, FakeTablet2);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet2],
            env.HiveProxyActorId);
    }

    Y_UNIT_TEST(LockReconnected)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        auto sender = runtime.AllocateEdgeActor();

        env.SendLockRequest(sender, FakeTablet2);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet2],
            env.HiveProxyActorId);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->ReconnectRequests[FakeTablet2],
            0);

        int hiveLockRequests = 0;
        runtime.SetObserverFunc(
            [&](TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                Y_UNUSED(runtime);
                if (event->GetTypeRewrite() == TEvHive::EvLockTabletExecution) {
                    ++hiveLockRequests;
                }
                return TTestActorRuntime::EEventAction::PROCESS;
            });

        env.EnableTabletResolverScheduling();
        env.RebootHive();

        for (int retries = 0; retries < 5 && !hiveLockRequests; ++retries) {
            // Pipe to hive may take a long time to connect
            // Wait until hive receives the lock request
            runtime.DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));
        }

        runtime.SetObserverFunc(&TTestActorRuntime::DefaultObserverFunc);

        // Rebooting hive should reconnect the lock
        UNIT_ASSERT_VALUES_EQUAL(hiveLockRequests, 1);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet2],
            env.HiveProxyActorId);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->ReconnectRequests[FakeTablet2],
            1);

        // Unlock should succeed, since lock isn't lost
        env.SendUnlockRequest(sender, FakeTablet2);
        UNIT_ASSERT_VALUES_EQUAL(
            env.HiveState->LockedTablets[FakeTablet2],
            TActorId());
    }

    Y_UNIT_TEST(GetStorageInfoMissing)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        auto sender = runtime.AllocateEdgeActor();

        auto result = env.SendGetStorageInfoRequest(
            sender,
            FakeTablet2,
            MAKE_KIKIMR_ERROR(NKikimrProto::ERROR));

        UNIT_ASSERT(!result);
    }

    Y_UNIT_TEST(GetStorageInfoOK)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        TTabletStorageInfoPtr expected =
            CreateTestTabletInfo(FakeTablet2, TTabletTypes::BlockStoreVolume);
        env.HiveState->StorageInfos[FakeTablet2] = expected;

        auto sender = runtime.AllocateEdgeActor();

        auto result = env.SendGetStorageInfoRequest(sender, FakeTablet2);
        UNIT_ASSERT(result);
        UNIT_ASSERT_VALUES_EQUAL(result->TabletID, expected->TabletID);
        UNIT_ASSERT_VALUES_EQUAL(result->TabletType, expected->TabletType);
    }

    Y_UNIT_TEST(ReassignTablet)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        TTabletStorageInfoPtr expected =
            CreateTestTabletInfo(FakeTablet2, TTabletTypes::BlockStoreVolume);
        env.HiveState->StorageInfos[FakeTablet2] =
            new TTabletStorageInfo(*expected);

        auto sender = runtime.AllocateEdgeActor();

        env.SendReassignTabletRequest(
            sender,
            FakeTablet2,
            {1, 3},
            S_OK
        );

        auto result = env.HiveState->StorageInfos[FakeTablet2];

        UNIT_ASSERT(result);
        UNIT_ASSERT_VALUES_EQUAL(
            result->Channels.size(),
            expected->Channels.size()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            result->Channels[0].History.size(),
            expected->Channels[0].History.size());
        UNIT_ASSERT_VALUES_EQUAL(
            result->Channels[1].History.size(),
            expected->Channels[1].History.size() + 1
        );
        UNIT_ASSERT_VALUES_EQUAL(
            result->Channels[1].History.back().GroupID,
            expected->Channels[1].History.back().GroupID + 1
        );
        UNIT_ASSERT_VALUES_EQUAL(
            result->Channels[2].History.size(),
            expected->Channels[2].History.size());
        UNIT_ASSERT_VALUES_EQUAL(
            result->Channels[3].History.size(),
            expected->Channels[3].History.size() + 1
        );
        UNIT_ASSERT_VALUES_EQUAL(
            result->Channels[3].History.back().GroupID,
            expected->Channels[3].History.back().GroupID + 1
        );
    }

    Y_UNIT_TEST(ReassignTabletDuringDisconnect)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        TTabletStorageInfoPtr expected =
            CreateTestTabletInfo(FakeTablet2, TTabletTypes::BlockStoreVolume);
        env.HiveState->StorageInfos[FakeTablet2] =
            new TTabletStorageInfo(*expected);

        auto sender = runtime.AllocateEdgeActor();

        runtime.SetObserverFunc(
            [&](TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                Y_UNUSED(runtime);
                if (event->GetTypeRewrite() == TEvHive::EvReassignTablet) {
                    env.RebootHive();
                    return TTestActorRuntime::EEventAction::DROP;
                }

                return TTestActorRuntime::EEventAction::PROCESS;
            });

        env.SendReassignTabletRequest(
            sender,
            FakeTablet2,
            {1, 3},
            E_REJECTED
        );
    }

    Y_UNIT_TEST(BootExternalError)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        auto sender = runtime.AllocateEdgeActor();

        auto result = env.SendBootExternalRequest(
            sender,
            FakeTablet2,
            MAKE_KIKIMR_ERROR(NKikimrProto::ERROR));
        UNIT_ASSERT(!result.StorageInfo);
    }

    Y_UNIT_TEST(BootExternalOK)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        TTabletStorageInfoPtr expected =
            CreateTestTabletInfo(FakeTablet2, TTabletTypes::BlockStorePartition);
        env.HiveState->StorageInfos[FakeTablet2] = expected;

        auto sender = runtime.AllocateEdgeActor();

        auto result = env.SendBootExternalRequest(sender, FakeTablet2, S_OK);
        UNIT_ASSERT(result.StorageInfo);
        UNIT_ASSERT_VALUES_EQUAL(result.StorageInfo->TabletID, expected->TabletID);
        UNIT_ASSERT_VALUES_EQUAL(result.StorageInfo->TabletType, expected->TabletType);
        UNIT_ASSERT_VALUES_EQUAL(result.SuggestedGeneration, 1u);

        auto result2 = env.SendBootExternalRequest(sender, FakeTablet2, S_OK);
        UNIT_ASSERT(result2.StorageInfo);
        UNIT_ASSERT_VALUES_EQUAL(result2.SuggestedGeneration, 2u);
    }

    Y_UNIT_TEST(DrainNode)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        auto sender = runtime.AllocateEdgeActor();

        env.SendDrainNodeRequest(sender, false, E_FAIL);

        env.HiveState->DrainableNodeIds.insert(sender.NodeId());
        env.SendDrainNodeRequest(sender, false, S_OK);

        UNIT_ASSERT(!env.HiveState->DownNodeIds.contains(sender.NodeId()));

        env.SendDrainNodeRequest(sender, true, S_OK);
        UNIT_ASSERT(env.HiveState->DownNodeIds.contains(sender.NodeId()));
    }

    Y_UNIT_TEST(BootExternalInFallbackMode)
    {
        TString cacheFilePath =
            "BootExternalInFallbackMode.tablet_boot_info_cache.txt";;
        bool fallbackMode = false;

        {
            TTestBasicRuntime runtime;
            TTestEnv env(runtime, cacheFilePath, fallbackMode);

            TTabletStorageInfoPtr expected = CreateTestTabletInfo(
                FakeTablet2,
                TTabletTypes::BlockStorePartition);
            env.HiveState->StorageInfos[FakeTablet2] = expected;

            auto sender = runtime.AllocateEdgeActor();

            auto result = env.SendBootExternalRequest(sender, FakeTablet2, S_OK);
            UNIT_ASSERT(result.StorageInfo);
            UNIT_ASSERT_VALUES_EQUAL(
                FakeTablet2,
                result.StorageInfo->TabletID);
            UNIT_ASSERT_VALUES_EQUAL(1u, result.SuggestedGeneration);

            env.SendSyncTabletBootInfoCache(sender, S_OK);
        }

        fallbackMode = true;
        {
            TTestBasicRuntime runtime;
            TTestEnv env(runtime, cacheFilePath, fallbackMode);

            auto sender = runtime.AllocateEdgeActor();

            auto result1 = env.SendBootExternalRequest(sender, FakeTablet2, S_OK);
            UNIT_ASSERT(result1.StorageInfo);
            UNIT_ASSERT_VALUES_EQUAL(
                FakeTablet2,
                result1.StorageInfo->TabletID);
            UNIT_ASSERT_VALUES_EQUAL(1u, result1.SuggestedGeneration);

            // unknown tablet should not be booted
            auto result2 = env.SendBootExternalRequest(
                sender, 0xdeadbeaf, E_REJECTED);
            UNIT_ASSERT(!result2.StorageInfo);
        }
    }
}

}   // namespace NCloud::NStorage
