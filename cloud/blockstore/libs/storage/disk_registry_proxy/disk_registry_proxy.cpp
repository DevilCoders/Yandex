#include "disk_registry_proxy.h"

#include "config.h"

#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <cloud/storage/core/libs/api/hive_proxy.h>

#include <ydb/core/base/appdata.h>
#include <ydb/core/mon/mon.h>
#include <ydb/core/tablet/tablet_pipe_client_cache.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/monlib/service/pages/templates.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

using namespace NCloud::NStorage;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr ui32 DrChannelCount = 3;

////////////////////////////////////////////////////////////////////////////////

struct TDiskRegistryChannelKinds
{
    TString SysKind;
    TString LogKind;
    TString IndexKind;
};

std::array<TString, DrChannelCount> GetPoolKinds(
    const TDiskRegistryChannelKinds& kinds,
    const TStorageConfig& config)
{
    return {
        kinds.SysKind ? kinds.SysKind : config.GetSSDSystemChannelPoolKind(),
        kinds.LogKind ? kinds.LogKind : config.GetSSDLogChannelPoolKind(),
        kinds.IndexKind ? kinds.IndexKind : config.GetSSDIndexChannelPoolKind(),
    };
}

////////////////////////////////////////////////////////////////////////////////

ui64 GetHiveTabletId(const TActorContext& ctx)
{
    auto& domainsInfo = *AppData(ctx)->DomainsInfo;
    Y_VERIFY(domainsInfo.Domains);
    auto domainUid = domainsInfo.Domains.begin()->first;
    auto hiveUid = domainsInfo.GetDefaultHiveUid(domainUid);
    return domainsInfo.GetHive(hiveUid);
}

////////////////////////////////////////////////////////////////////////////////

class TCreateDiskRegistryActor final
    : public TActorBootstrapped<TCreateDiskRegistryActor>
{
private:
    const TStorageConfigPtr StorageConfig;
    const TDiskRegistryProxyConfigPtr Config;
    const TActorId Sender;
    const TDiskRegistryChannelKinds Kinds;

public:
    TCreateDiskRegistryActor(
        TStorageConfigPtr config,
        TDiskRegistryProxyConfigPtr diskRegistryProxyConfig,
        TActorId requester,
        TDiskRegistryChannelKinds kinds);

    void Bootstrap(const TActorContext& ctx);

private:
    void ReplyAndDie(const TActorContext& ctx, ui64 tabletId);
    void ReplyAndDie(const TActorContext& ctx, NProto::TError error);

private:
    STFUNC(StateWork);

    void HandleCreateTablet(
        TEvHiveProxy::TEvCreateTabletResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleDescribeSchemeResponse(
        TEvSSProxy::TEvDescribeSchemeResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TCreateDiskRegistryActor::TCreateDiskRegistryActor(
        TStorageConfigPtr config,
        TDiskRegistryProxyConfigPtr diskRegistryProxyConfig,
        TActorId sender,
        TDiskRegistryChannelKinds kinds)
    : StorageConfig(std::move(config))
    , Config(std::move(diskRegistryProxyConfig))
    , Sender(std::move(sender))
    , Kinds(std::move(kinds))
{
    ActivityType = TBlockStoreActivities::DISK_REGISTRY_PROXY;
}

void TCreateDiskRegistryActor::Bootstrap(const TActorContext& ctx)
{
    TThis::Become(&TThis::StateWork);

    auto request = std::make_unique<TEvSSProxy::TEvDescribeSchemeRequest>(
        StorageConfig->GetSchemeShardDir());

    NCloud::Send(
        ctx,
        MakeSSProxyServiceId(),
        std::move(request),
        0);
}

void TCreateDiskRegistryActor::ReplyAndDie(const TActorContext& ctx, ui64 tabletId)
{
    auto response = std::make_unique<TEvDiskRegistryProxy::TEvDiskRegistryCreateResult>(
        tabletId);
    NCloud::Send(ctx, Sender, std::move(response));

    Die(ctx);
}

void TCreateDiskRegistryActor::ReplyAndDie(const TActorContext& ctx, NProto::TError error)
{
    auto response = std::make_unique<TEvDiskRegistryProxy::TEvDiskRegistryCreateResult>(
        error);
    NCloud::Send(ctx, Sender, std::move(response));

    Die(ctx);
}

void TCreateDiskRegistryActor::HandleDescribeSchemeResponse(
    TEvSSProxy::TEvDescribeSchemeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    if (ev->Cookie) {
        LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
            "Received unexpected DescribeScheme response");

        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Received DescribeScheme response");

    auto* msg = ev->Get();

    if (HasError(msg->Error)) {
        LOG_ERROR_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
            "Can't create Disk Registry tablet. Error on describe scheme: "
            << FormatError(msg->Error));

        ReplyAndDie(ctx, std::move(msg->Error));
        return;
    }

    const auto& descr = msg->PathDescription.GetDomainDescription();

    if (descr.StoragePoolsSize() == 0) {
        LOG_ERROR_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
            "Can't create Disk Registry tablet. Empty storage pool");

        ReplyAndDie(
            ctx,
            MakeError(E_FAIL, "Can't create Disk Registry tablet. Empty storage pool"));
        return;
    }

    NKikimrHive::TEvCreateTablet request;

    request.SetOwner(Config->GetOwner());
    request.SetOwnerIdx(Config->GetOwnerIdx());
    request.SetTabletType(TTabletTypes::BlockStoreDiskRegistry);

    *request.AddAllowedDomains() = descr.GetDomainKey();

    for (const auto& channelKind: GetPoolKinds(Kinds, *StorageConfig)) {
        auto* pool = FindIfPtr(descr.GetStoragePools(), [&] (const auto& pool) {
            return pool.GetKind() == channelKind;
        });
        if (!pool) {
            TStringBuilder pools;
            for (const auto& pool: descr.GetStoragePools()) {
                pools << '[' << pool.GetName() << "," << pool.GetKind() << ']';
            }

            auto b = TStringBuilder() <<
                "Can't create Disk Registry tablet. No pool for " <<
                channelKind.Quote() <<
                " but only " <<
                pools <<
                " available";

            LOG_ERROR_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY, b);

            ReplyAndDie(ctx, MakeError(E_FAIL, b));
            return;
        }
        request.AddBindedChannels()->SetStoragePoolName(pool->GetName());
    }

    const auto hiveTabletId = GetHiveTabletId(ctx);

    LOG_INFO_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Create Disk Registry Tablet. Hive: " << hiveTabletId);

    NCloud::Send<TEvHiveProxy::TEvCreateTabletRequest>(
        ctx,
        MakeHiveProxyServiceId(),
        ev->Cookie,
        hiveTabletId,
        std::move(request));

    return;
}

void TCreateDiskRegistryActor::HandleCreateTablet(
    TEvHiveProxy::TEvCreateTabletResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    if (ev->Cookie) {
        LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
            "Received expired CreateTablet response");

        return;
    }

    if (!HasError(msg->Error)) {
        ReplyAndDie(ctx, msg->TabletId);
        return;
    }

    LOG_ERROR_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Can't create Disk Registry tablet: " << FormatError(msg->Error));

    ReplyAndDie(ctx, std::move(msg->Error));
}

STFUNC(TCreateDiskRegistryActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvDescribeSchemeResponse, HandleDescribeSchemeResponse);
        HFunc(TEvHiveProxy::TEvCreateTabletResponse, HandleCreateTablet);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::DISK_REGISTRY_PROXY);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

class TDiskRegistryProxyActor final
    : public TActorBootstrapped<TDiskRegistryProxyActor>
{
private:
    using TActiveRequests = THashMap<ui64, IEventHandlePtr>;

private:
    const TStorageConfigPtr StorageConfig;
    const TDiskRegistryProxyConfigPtr Config;

    ui64 DiskRegistryTabletId = 0;
    TActorId TabletClientId;

    ui64 RequestId = 0;

    TActiveRequests ActiveRequests;

    TDeque<TActorId> Subscribers;

    TRequestInfoPtr ReassignRequestInfo;

public:
    TDiskRegistryProxyActor(
        TStorageConfigPtr config,
        TDiskRegistryProxyConfigPtr diskRegistryProxyConfig);

    void Bootstrap(const TActorContext& ctx);

private:
    void CancelActiveRequests(const TActorContext& ctx);

    bool ReplyWithError(const TActorContext& ctx, TAutoPtr<IEventHandle>& ev);
    template <typename TMethod>
    void ReplyWithErrorImpl(const TActorContext& ctx, TAutoPtr<IEventHandle>& ev);

    void CreateClient(const TActorContext& ctx);

    void StartWork(ui64 tabletId, const TActorContext& ctx);

    void RegisterPages(const TActorContext& ctx);
    void RenderHtmlInfo(IOutputStream& out) const;

    void NotifySubscribers(const TActorContext& ctx);

    void LookupTablet(const TActorContext& ctx);

private:
    STFUNC(StateLookup);
    STFUNC(StateError);
    STFUNC(StateWork);

    void HandleConnected(
        TEvTabletPipe::TEvClientConnected::TPtr& ev,
        const TActorContext& ctx);

    void HandleDisconnect(
        TEvTabletPipe::TEvClientDestroyed::TPtr& ev,
        const TActorContext& ctx);

    void HandleRequest(
        const TActorContext& ctx,
        TAutoPtr<IEventHandle>& ev);

    void HandleResponse(
        const TActorContext& ctx,
        TAutoPtr<IEventHandle>& ev);

    void HandleCreateTablet(
        TEvHiveProxy::TEvCreateTabletResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleLookupTablet(
        TEvHiveProxy::TEvLookupTabletResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleHttpInfo(
        const NMon::TEvHttpInfo::TPtr& ev,
        const TActorContext& ctx);

    void HandleWakeup(
        const TEvents::TEvWakeup::TPtr& ev,
        const TActorContext& ctx);

    void HandleCreateResult(
        const TEvDiskRegistryProxy::TEvDiskRegistryCreateResult::TPtr& ev,
        const TActorContext& ctx);

    bool HandleRequests(STFUNC_SIG);
    bool LogLateMessage(STFUNC_SIG);

    BLOCKSTORE_DISK_REGISTRY_PROXY_REQUESTS(
        BLOCKSTORE_IMPLEMENT_REQUEST,
        TEvDiskRegistryProxy)
};

////////////////////////////////////////////////////////////////////////////////

TDiskRegistryProxyActor::TDiskRegistryProxyActor(
        TStorageConfigPtr config,
        TDiskRegistryProxyConfigPtr diskRegistryProxyConfig)
    : StorageConfig(std::move(config))
    , Config(std::move(diskRegistryProxyConfig))
{
    ActivityType = TBlockStoreActivities::DISK_REGISTRY_PROXY;
}

void TDiskRegistryProxyActor::Bootstrap(const TActorContext& ctx)
{
    if (!Config->GetOwner()) {
        LOG_WARN(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
            "Disk Registry Proxy not configured");

        TThis::Become(&TThis::StateError);
        return;
    }

    TThis::Become(&TThis::StateLookup);

    RegisterPages(ctx);

    LookupTablet(ctx);
}

void TDiskRegistryProxyActor::LookupTablet(const TActorContext& ctx)
{
    const auto hiveTabletId = GetHiveTabletId(ctx);

    LOG_INFO_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Lookup Disk Registry tablet. Hive: " << hiveTabletId);

    const ui64 cookie = ++RequestId;

    NCloud::Send<TEvHiveProxy::TEvLookupTabletRequest>(
        ctx,
        MakeHiveProxyServiceId(),
        cookie,
        hiveTabletId,
        Config->GetOwner(),
        Config->GetOwnerIdx());

    ctx.Schedule(Config->GetLookupTimeout(), new TEvents::TEvWakeup());
}

void TDiskRegistryProxyActor::StartWork(
    ui64 tabletId,
    const TActorContext& ctx)
{
    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Ready to work. Tablet ID: %lu", tabletId);

    TThis::Become(&TThis::StateWork);

    DiskRegistryTabletId = tabletId;
}

void TDiskRegistryProxyActor::CreateClient(const TActorContext& ctx)
{
    NTabletPipe::TClientConfig clientConfig;
    clientConfig.RetryPolicy = {
        .RetryLimitCount = StorageConfig->GetPipeClientRetryCount(),
        .MinRetryTime = StorageConfig->GetPipeClientMinRetryTime(),
        .MaxRetryTime = StorageConfig->GetPipeClientMaxRetryTime()
    };

    TabletClientId = ctx.Register(NTabletPipe::CreateClient(
        ctx.SelfID,
        DiskRegistryTabletId,
        clientConfig));

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Tablet client: %lu (remote: %s)",
        DiskRegistryTabletId,
        ToString(TabletClientId).data());
}

bool TDiskRegistryProxyActor::ReplyWithError(
    const TActorContext& ctx,
    TAutoPtr<IEventHandle>& ev)
{
#define BLOCKSTORE_HANDLE_METHOD(name, ns)                                     \
    case ns::TEv##name##Request::EventType: {                                  \
        ReplyWithErrorImpl<ns::T##name##Method>(ctx, ev);                      \
        return true;                                                           \
    }                                                                          \
// BLOCKSTORE_HANDLE_METHOD

    switch (ev->GetTypeRewrite()) {
        BLOCKSTORE_DISK_REGISTRY_REQUESTS_PROTO(BLOCKSTORE_HANDLE_METHOD,
                TEvDiskRegistry)
        BLOCKSTORE_DISK_REGISTRY_REQUESTS_FWD_SERVICE(BLOCKSTORE_HANDLE_METHOD,
                TEvService)
        BLOCKSTORE_DISK_REGISTRY_PROXY_REQUESTS(BLOCKSTORE_HANDLE_METHOD,
                TEvDiskRegistryProxy)
        default:
            return false;
    }
#undef BLOCKSTORE_HANDLE_METHOD
}

template <typename TMethod>
void TDiskRegistryProxyActor::ReplyWithErrorImpl(
    const TActorContext& ctx,
    TAutoPtr<IEventHandle>& ev)
{
    auto response = std::make_unique<typename TMethod::TResponse>(
        MakeError(E_REJECTED, "DiskRegistry tablet not available"));

    BLOCKSTORE_TRACE_SENT(ctx, &ev->TraceId, this, response);

    NCloud::Reply(ctx, *ev, std::move(response));
}

void TDiskRegistryProxyActor::CancelActiveRequests(const TActorContext& ctx)
{
    TActiveRequests activeRequests = std::move(ActiveRequests);

    for (auto& kv : activeRequests) {
        TAutoPtr<IEventHandle> ev(kv.second.release());

        if (!ReplyWithError(ctx, ev)) {
            Y_FAIL("Unexpected event: (0x%08X)", ev->GetTypeRewrite());
        }
    }
}

void TDiskRegistryProxyActor::RegisterPages(const TActorContext& ctx)
{
    auto mon = AppData(ctx)->Mon;
    if (mon) {
        auto* rootPage = mon->RegisterIndexPage("blockstore", "BlockStore");

        mon->RegisterActorPage(rootPage, "disk_registry_proxy", "DiskRegistryProxy",
            false, ctx.ExecutorThread.ActorSystem, SelfId());
    }
}

void TDiskRegistryProxyActor::NotifySubscribers(const TActorContext& ctx)
{
    for (const auto& subscriber: Subscribers) {
        auto request = std::make_unique<TEvDiskRegistryProxy::TEvConnectionLost>();

        NCloud::Send(ctx, subscriber, std::move(request));
    }
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryProxyActor::HandleConnected(
    TEvTabletPipe::TEvClientConnected::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    LOG_DEBUG_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Connection to Disk Registry ready. TabletId: " << msg->TabletId
            << " Status: " << NKikimrProto::EReplyStatus_Name(msg->Status)
            << " ClientId: " << msg->ClientId
            << " ServerId: " << msg->ServerId
            << " Master: " << msg->Leader
            << " Dead: " << msg->Dead);

    if (!msg->ServerId) {
        LOG_ERROR_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
            "Cannot connect to Disk Registry tablet " << msg->TabletId << ": "
                << NKikimrProto::EReplyStatus_Name(msg->Status));

        TabletClientId = {};
        CancelActiveRequests(ctx);
    }
}

void TDiskRegistryProxyActor::HandleDisconnect(
    TEvTabletPipe::TEvClientDestroyed::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    Y_VERIFY(DiskRegistryTabletId);

    TabletClientId = {};

    const auto error = MakeError(E_REJECTED, "Connection broken");

    LOG_WARN(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Connection to Disk Registry failed: %s",
        FormatError(error).data());

    NotifySubscribers(ctx);

    CancelActiveRequests(ctx);
}

void TDiskRegistryProxyActor::HandleRequest(
    const TActorContext& ctx,
    TAutoPtr<IEventHandle>& ev)
{
    Y_VERIFY(DiskRegistryTabletId);

    if (!TabletClientId) {
        CreateClient(ctx);
    }

    const ui64 requestId = ++RequestId;

    auto event = std::make_unique<IEventHandle>(
        ev->Recipient,
        SelfId(),
        ev->ReleaseBase().Release(),
        0,          // flags
        requestId,  // cookie
        nullptr,    // forwardOnNondelivery
        ev->TraceId.Clone());

    event->Rewrite(TEvTabletPipe::EvSend, TabletClientId);
    ctx.Send(event.release());

    ActiveRequests.emplace(requestId, IEventHandlePtr(ev.Release()));
}

void TDiskRegistryProxyActor::HandleResponse(
    const TActorContext& ctx,
    TAutoPtr<IEventHandle>& ev)
{
    auto it = ActiveRequests.find(ev->Cookie);
    if (it == ActiveRequests.end()) {
        // ActiveRequests are cleared upon connection reset
        if (!LogLateMessage(ev, ctx)) {
            LogUnexpectedEvent(ctx, ev,
                TBlockStoreComponents::DISK_REGISTRY_PROXY);
        }
        return;
    }

    IEventHandle& request = *it->second;

    // forward response to the caller
    TAutoPtr<IEventHandle> event;
    if (ev->HasEvent()) {
        event = new IEventHandle(
            request.Sender,
            ev->Sender,
            ev->ReleaseBase().Release(),
            ev->Flags,
            request.Cookie,
            nullptr,        // undeliveredRequestActor
            std::move(ev->TraceId));
    } else {
        event = new IEventHandle(
            ev->Type,
            ev->Flags,
            request.Sender,
            ev->Sender,
            ev->ReleaseChainBuffer(),
            request.Cookie,
            nullptr,    // undeliveredRequestActor
            std::move(ev->TraceId));
    }

    ctx.Send(event);
    ActiveRequests.erase(it);
}

bool TDiskRegistryProxyActor::HandleRequests(STFUNC_SIG)
{
#define BLOCKSTORE_HANDLE_METHOD(name, ns)                                     \
    case ns::TEv##name##Request::EventType: {                                  \
        HandleRequest(ctx, ev);                                                \
        break;                                                                 \
    }                                                                          \
    case ns::TEv##name##Response::EventType: {                                 \
        HandleResponse(ctx, ev);                                               \
        break;                                                                 \
    }                                                                          \
// BLOCKSTORE_HANDLE_METHOD

    switch (ev->GetTypeRewrite()) {
        BLOCKSTORE_DISK_REGISTRY_REQUESTS_PROTO(BLOCKSTORE_HANDLE_METHOD,
            TEvDiskRegistry)
        BLOCKSTORE_DISK_REGISTRY_REQUESTS_FWD_SERVICE(BLOCKSTORE_HANDLE_METHOD,
            TEvService)

        default:
            return false;
    }

    return true;

#undef BLOCKSTORE_HANDLE_METHOD
}

bool TDiskRegistryProxyActor::LogLateMessage(STFUNC_SIG)
{
#define BLOCKSTORE_LOG_MESSAGE(name, ns)                                       \
    case ns::TEv##name##Request::EventType: {                                  \
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,             \
            "Late request : (0x%08X) %s request",                              \
            ev->GetTypeRewrite(),                                              \
            #name);                                                            \
        break;                                                                 \
    }                                                                          \
    case ns::TEv##name##Response::EventType: {                                 \
        LOG_WARN(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,             \
          "Late response : (0x%08X) %s response",                              \
          ev->GetTypeRewrite(),                                                \
          #name);                                                              \
        break;                                                                 \
    }                                                                          \
// BLOCKSTORE_LOG_MESSAGE

    switch (ev->GetTypeRewrite()) {
        BLOCKSTORE_DISK_REGISTRY_REQUESTS_PROTO(BLOCKSTORE_LOG_MESSAGE,
            TEvDiskRegistry)
        BLOCKSTORE_DISK_REGISTRY_REQUESTS_FWD_SERVICE(BLOCKSTORE_LOG_MESSAGE,
            TEvService)

        default:
            return false;
    }

    return true;

#undef BLOCKSTORE_LOG_MESSAGE
}

void TDiskRegistryProxyActor::HandleLookupTablet(
    TEvHiveProxy::TEvLookupTabletResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (!HasError(msg->Error)) {
        StartWork(msg->TabletId, ctx);
        return;
    }

    if (ev->Cookie != RequestId) {
        LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
            "Received expired LookupTablet response");

        return;
    }

    LOG_WARN_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Can't find Disk Registry tablet: " << FormatError(msg->Error)
            << ". Will try to create it");

    NCloud::Register<TCreateDiskRegistryActor>(
        ctx,
        StorageConfig,
        Config,
        SelfId(),
        TDiskRegistryChannelKinds());
}

void TDiskRegistryProxyActor::RenderHtmlInfo(IOutputStream& out) const
{
    using namespace NMonitoringUtils;

    HTML(out) {
        out << "Disk Registry Tablet: ";
        if (DiskRegistryTabletId) {
            out << "<a href='../tablets?TabletID="
                << DiskRegistryTabletId << "'>"
                << DiskRegistryTabletId << "</a>";
        }

        H3() { out << "Config"; }
        Config->DumpHtml(out);
    }
}

void TDiskRegistryProxyActor::HandleHttpInfo(
    const NMon::TEvHttpInfo::TPtr& ev,
    const TActorContext& ctx)
{
    LOG_DEBUG_S(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "HTTP request: " << ev->Get()->Request.GetUri());

    TStringStream out;
    RenderHtmlInfo(out);

    NCloud::Reply(
        ctx,
        *ev,
        std::make_unique<NMon::TEvHttpInfoRes>(out.Str()));
}

void TDiskRegistryProxyActor::HandleSubscribe(
    const TEvDiskRegistryProxy::TEvSubscribeRequest::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    NProto::TError error;

    auto* msg = ev->Get();
    auto it = Find(Subscribers, msg->Subscriber);
    if (it == Subscribers.end()) {
        Subscribers.push_back(msg->Subscriber);
    } else {
        error.SetCode(S_ALREADY);
    }

    const bool connected = !!TabletClientId;

    auto response = std::make_unique<TEvDiskRegistryProxy::TEvSubscribeResponse>(
        std::move(error), connected);

    NCloud::Reply(ctx, *ev, std::move(response));
}

void TDiskRegistryProxyActor::HandleUnsubscribe(
    const TEvDiskRegistryProxy::TEvUnsubscribeRequest::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    NProto::TError error;

    auto* msg = ev->Get();
    auto it = Find(Subscribers, msg->Subscriber);
    if (it != Subscribers.end()) {
        Subscribers.erase(it);
    } else {
        error.SetCode(S_FALSE);
    }

    auto response = std::make_unique<TEvDiskRegistryProxy::TEvUnsubscribeResponse>(
        std::move(error));

    NCloud::Reply(ctx, *ev, std::move(response));
}

void TDiskRegistryProxyActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY_PROXY,
        "Lookup/Create tablet request timed out. Retry.");

    LookupTablet(ctx);
}

void TDiskRegistryProxyActor::HandleReassign(
    const TEvDiskRegistryProxy::TEvReassignRequest::TPtr& ev,
    const TActorContext& ctx)
{
    if (ReassignRequestInfo) {
        auto response = std::make_unique<TEvDiskRegistryProxy::TEvReassignResponse>(
            MakeError(E_REJECTED, "Disk Registry reassign is in progress"));

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    const auto* msg = ev->Get();

    ReassignRequestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TDiskRegistryChannelKinds kinds = {
        std::move(msg->SysKind),
        std::move(msg->LogKind),
        std::move(msg->IndexKind)
    };

    NCloud::Register<TCreateDiskRegistryActor>(
        ctx,
        StorageConfig,
        Config,
        SelfId(),
        std::move(kinds));
}

void TDiskRegistryProxyActor::HandleCreateResult(
    const TEvDiskRegistryProxy::TEvDiskRegistryCreateResult::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    if (!ReassignRequestInfo) {
        if (!FAILED(msg->GetStatus())) {
            StartWork(msg->TabletId, ctx);
        } else {
            TThis::Become(&TThis::StateError);
        }
        return;
    }

    auto response = std::make_unique<TEvDiskRegistryProxy::TEvReassignResponse>(
        std::move(msg->Error)
    );

    NCloud::Reply(ctx, *ReassignRequestInfo, std::move(response));
    ReassignRequestInfo.Reset();
}

STFUNC(TDiskRegistryProxyActor::StateError)
{
    switch (ev->GetTypeRewrite()) {
        IgnoreFunc(TEvents::TEvWakeup);

        HFunc(NMon::TEvHttpInfo, HandleHttpInfo);

        IgnoreFunc(TEvHiveProxy::TEvLookupTabletResponse);
        IgnoreFunc(TEvHiveProxy::TEvCreateTabletResponse);

        default:
            if (!ReplyWithError(ctx, ev)) {
                HandleUnexpectedEvent(ctx, ev,
                    TBlockStoreComponents::DISK_REGISTRY_PROXY);
            }
            break;
    }
}

STFUNC(TDiskRegistryProxyActor::StateLookup)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvHiveProxy::TEvLookupTabletResponse, HandleLookupTablet);
        HFunc(TEvDiskRegistryProxy::TEvDiskRegistryCreateResult, HandleCreateResult);

        HFunc(TEvents::TEvWakeup, HandleWakeup);

        HFunc(NMon::TEvHttpInfo, HandleHttpInfo);

        HFunc(TEvDiskRegistryProxy::TEvSubscribeRequest, HandleSubscribe);
        HFunc(TEvDiskRegistryProxy::TEvUnsubscribeRequest, HandleUnsubscribe);

        default:
            if (!ReplyWithError(ctx, ev)) {
                HandleUnexpectedEvent(ctx, ev,
                    TBlockStoreComponents::DISK_REGISTRY_PROXY);
            }
            break;
    }
}

STFUNC(TDiskRegistryProxyActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvTabletPipe::TEvClientConnected, HandleConnected);
        HFunc(TEvTabletPipe::TEvClientDestroyed, HandleDisconnect);

        IgnoreFunc(TEvents::TEvWakeup);

        HFunc(NMon::TEvHttpInfo, HandleHttpInfo);

        IgnoreFunc(TEvHiveProxy::TEvLookupTabletResponse);
        HFunc(TEvDiskRegistryProxy::TEvDiskRegistryCreateResult, HandleCreateResult);

        HFunc(TEvDiskRegistryProxy::TEvSubscribeRequest, HandleSubscribe);
        HFunc(TEvDiskRegistryProxy::TEvUnsubscribeRequest, HandleUnsubscribe);

        HFunc(TEvDiskRegistryProxy::TEvReassignRequest, HandleReassign);

        default:
            if (!HandleRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev,
                    TBlockStoreComponents::DISK_REGISTRY_PROXY);
            }
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr CreateDiskRegistryProxy(
    TStorageConfigPtr storageConfig,
    TDiskRegistryProxyConfigPtr proxyConfig)
{
    return std::make_unique<TDiskRegistryProxyActor>(
        std::move(storageConfig),
        std::move(proxyConfig));
}

}   // namespace NCloud::NBlockStore::NStorage
