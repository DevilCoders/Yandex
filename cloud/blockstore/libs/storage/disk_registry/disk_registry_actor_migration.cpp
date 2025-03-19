#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TMarkReplacementDevicesActor final
    : public TActorBootstrapped<TMarkReplacementDevicesActor>
{
private:
    const TActorId Owner;
    const TRequestInfoPtr RequestInfo;
    const NProto::TFinishMigrationRequest Request;

    NProto::TError Error;
    int PendingOperations = 0;

public:
    TMarkReplacementDevicesActor(
        const TActorId& owner,
        TRequestInfoPtr requestInfo,
        NProto::TFinishMigrationRequest request);

    void Bootstrap(const TActorContext& ctx);

private:
    void MarkReplacementDevices(const TActorContext& ctx);
    void ReplyAndDie(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void HandleResponse(
        const TEvDiskRegistry::TEvMarkReplacementDeviceResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TMarkReplacementDevicesActor::TMarkReplacementDevicesActor(
        const TActorId& owner,
        TRequestInfoPtr requestInfo,
        NProto::TFinishMigrationRequest request)
    : Owner(owner)
    , RequestInfo(std::move(requestInfo))
    , Request(std::move(request))
{
    ActivityType = TBlockStoreActivities::DISK_REGISTRY_WORKER;
}

void TMarkReplacementDevicesActor::Bootstrap(const TActorContext& ctx)
{
    Become(&TThis::StateWork);

    MarkReplacementDevices(ctx);
}

void TMarkReplacementDevicesActor::ReplyAndDie(const TActorContext& ctx)
{
    auto response = std::make_unique<TEvDiskRegistry::TEvFinishMigrationResponse>(
        std::move(Error));

    NCloud::Reply(ctx, *RequestInfo, std::move(response));

    NCloud::Send(
        ctx,
        Owner,
        std::make_unique<TEvDiskRegistryPrivate::TEvOperationCompleted>());
    Die(ctx);
}

void TMarkReplacementDevicesActor::MarkReplacementDevices(const TActorContext& ctx)
{
    PendingOperations = Request.MigrationsSize();

    ui64 cookie = 0;
    for (const auto& m: Request.GetMigrations()) {
        auto request =
            std::make_unique<TEvDiskRegistry::TEvMarkReplacementDeviceRequest>();
        request->Record.SetDiskId(Request.GetDiskId());
        request->Record.SetDeviceId(m.GetTargetDeviceId());
        NCloud::Send(
            ctx,
            Owner,
            std::move(request),
            cookie++
        );
    }
}

////////////////////////////////////////////////////////////////////////////////

void TMarkReplacementDevicesActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    Error = MakeError(E_REJECTED, "Tablet is dead");
    ReplyAndDie(ctx);
}

void TMarkReplacementDevicesActor::HandleResponse(
    const TEvDiskRegistry::TEvMarkReplacementDeviceResponse::TPtr& ev,
    const TActorContext& ctx)
{
    --PendingOperations;

    Y_VERIFY(PendingOperations >= 0);

    if (HasError(ev->Get()->Record)) {
        Error = std::move(*ev->Get()->Record.MutableError());
    }

    if (!PendingOperations) {
        ReplyAndDie(ctx);
    }
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TMarkReplacementDevicesActor::StateWork)
{
    Y_UNUSED(ctx);

    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvDiskRegistry::TEvMarkReplacementDeviceResponse, HandleResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::DISK_REGISTRY_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleFinishMigration(
    const TEvDiskRegistry::TEvFinishMigrationRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(FinishMigration);

    auto& record = ev->Get()->Record;

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received FinishMigration request: DiskId=%s, Migrations=%d",
        TabletID(),
        record.GetDiskId().c_str(),
        record.MigrationsSize());

    auto requestInfo = CreateRequestInfo<TEvDiskRegistry::TFinishMigrationMethod>(
        ev->Sender,
        ev->Cookie,
        ev->Get()->CallContext,
        std::move(ev->TraceId));

    if (State->IsMasterDisk(record.GetDiskId())) {
        auto actor = NCloud::Register<TMarkReplacementDevicesActor>(
            ctx,
            SelfId(),
            std::move(requestInfo),
            std::move(record)
        );
        Actors.insert(actor);

        return;
    }

    ExecuteTx<TFinishMigration>(
        ctx,
        std::move(requestInfo),
        record.GetDiskId(),
        std::move(*record.MutableMigrations()),
        ctx.Now()
    );
}

////////////////////////////////////////////////////////////////////////////////

bool TDiskRegistryActor::PrepareFinishMigration(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TFinishMigration& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteFinishMigration(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TFinishMigration& args)
{
    TDiskRegistryDatabase db(tx.DB);
    for (auto& x: args.Migrations) {
        auto error = State->FinishDeviceMigration(
            db,
            args.DiskId,
            x.GetSourceDeviceId(),
            x.GetTargetDeviceId(),
            args.Timestamp,
            &args.AffectedDisk);

        if (HasError(error)) {
            LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
                "FinishDeviceMigration error: %s. DiskId=%s Source=%s Target=%s",
                FormatError(error).c_str(),
                args.DiskId.c_str(),
                x.GetSourceDeviceId().c_str(),
                x.GetTargetDeviceId().c_str());
        } else {
            LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
                "FinishDeviceMigration succeeded. DiskId=%s Source=%s Target=%s",
                args.DiskId.c_str(),
                x.GetSourceDeviceId().c_str(),
                x.GetTargetDeviceId().c_str());
        }

        if (!HasError(args.Error)) {
            args.Error = error;
        }
    }
}

void TDiskRegistryActor::CompleteFinishMigration(
    const TActorContext& ctx,
    TTxDiskRegistry::TFinishMigration& args)
{
    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "FinishMigration complete. DiskId=%s Migrations=%d",
        args.DiskId.c_str(),
        args.Migrations.size());

    NotifyDisks(ctx);
    NotifyUsers(ctx);
    PublishDiskStates(ctx);
    SecureErase(ctx);

    auto response = std::make_unique<TEvDiskRegistry::TEvFinishMigrationResponse>(
        std::move(args.Error));

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::StartMigration(const NActors::TActorContext& ctx)
{
    if (StartMigrationInProgress) {
        return;
    }

    if (State->IsMigrationListEmpty()) {
        return;
    }

    StartMigrationInProgress = true;

    auto request = std::make_unique<TEvDiskRegistryPrivate::TEvStartMigrationRequest>();

    auto deadline = Min(StartMigrationStartTs, ctx.Now()) + TDuration::Seconds(5);
    if (deadline > ctx.Now()) {
        LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "[%lu] Scheduled device migration, now: %lu, deadline: %lu",
            TabletID(),
            ctx.Now().MicroSeconds(),
            deadline.MicroSeconds());

        ctx.ExecutorThread.Schedule(
            deadline,
            new IEventHandle(ctx.SelfID, ctx.SelfID, request.release()));
    } else {
        LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "[%lu] Sending device migration request",
            TabletID());

        NCloud::Send(ctx, ctx.SelfID, std::move(request));
    }
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleStartMigration(
    const TEvDiskRegistryPrivate::TEvStartMigrationRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(StartMigration);

    StartMigrationStartTs = ctx.Now();

    ExecuteTx<TStartMigration>(
        ctx,
        CreateRequestInfo<TEvDiskRegistryPrivate::TStartMigrationMethod>(
            ev->Sender,
            ev->Cookie,
            ev->Get()->CallContext,
            std::move(ev->TraceId)
        )
    );
}

////////////////////////////////////////////////////////////////////////////////

bool TDiskRegistryActor::PrepareStartMigration(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TStartMigration& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteStartMigration(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TStartMigration& args)
{
    Y_UNUSED(args);

    TDiskRegistryDatabase db(tx.DB);

    for (const auto& [diskId, deviceId]: State->BuildMigrationList()) {
        const auto result = State->StartDeviceMigration(db, diskId, deviceId);

        if (HasError(result)) {
            LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
                "[%lu] Start migration failed. DiskId=%s DeviceId=%s Error=%s",
                TabletID(),
                diskId.c_str(),
                deviceId.c_str(),
                FormatError(result.GetError()).c_str()
            );
        } else {
            const auto& target = result.GetResult();
            LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
                "[%lu] Start migration success. DiskId=%s DeviceId=%s TargetId={ %s %u %ul(%ul) }",
                TabletID(),
                diskId.c_str(),
                deviceId.c_str(),
                target.GetDeviceUUID().c_str(),
                target.GetBlockSize(),
                target.GetBlocksCount(),
                target.GetUnadjustedBlockCount()
            );
        }
    }
}

void TDiskRegistryActor::CompleteStartMigration(
    const TActorContext& ctx,
    TTxDiskRegistry::TStartMigration& args)
{
    NotifyDisks(ctx);
    NotifyUsers(ctx);

    NCloud::Reply(
        ctx,
        *args.RequestInfo,
        std::make_unique<TEvDiskRegistryPrivate::TEvStartMigrationResponse>());
}

void TDiskRegistryActor::HandleStartMigrationResponse(
    const TEvDiskRegistryPrivate::TEvStartMigrationResponse::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    StartMigrationInProgress = false;
    StartMigration(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage
