#pragma once

#include "public.h"

#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/storage/core/probes.h>

#include <cloud/storage/core/libs/kikimr/public.h>

#include <ydb/core/base/tablet.h>
#include <ydb/core/tablet_flat/flat_database.h>
#include <ydb/core/tablet_flat/tablet_flat_executed.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/actors/wilson/wilson_event.h>

namespace NCloud::NFileStore::NStorage {

LWTRACE_USING(FILESTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

NKikimr::NTabletFlatExecutor::IMiniKQLFactory* NewMiniKQLFactory();

////////////////////////////////////////////////////////////////////////////////

struct ITransactionBase
    : public NKikimr::NTabletFlatExecutor::ITransaction
{
    virtual void Init(const NActors::TActorContext& ctx) = 0;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class TTabletBase
    : public NKikimr::NTabletFlatExecutor::TTabletExecutedFlat
{
public:
    TTabletBase(const NActors::TActorId& owner,
                NKikimr::TTabletStorageInfoPtr storage)
        : TTabletExecutedFlat(storage.Get(), owner, NewMiniKQLFactory())
    {}

protected:
    template <typename TTx>
    class TTransaction final
        : public ITransactionBase
    {
    private:
        T* Self;
        typename TTx::TArgs Args;

        ui32 Generation = 0;
        ui32 Step = 0;

    public:
        template <typename ...TArgs>
        TTransaction(T* self, TArgs&& ...args)
            : Self(self)
            , Args(std::forward<TArgs>(args)...)
        {}

        NKikimr::TTxType GetTxType() const override
        {
            return TTx::TxType;
        }

        void Init(const NActors::TActorContext& ctx) override
        {
            Y_UNUSED(ctx);

            // FILESTORE_TRACE_EVENT(ctx, "Init", TracePtr(), Self, &Args, nullptr);
            if (auto* cc = CallContext()) {
                FILESTORE_TRACK(
                    TxInit,
                    cc,
                    TTx::Name);
            }
        }

        bool Execute(
            NKikimr::NTabletFlatExecutor::TTransactionContext& tx,
            const NActors::TActorContext& ctx) override
        {
            Generation = tx.Generation;
            Step = tx.Step;

            // FILESTORE_TRACE_EVENT(ctx, "PrepareTx", TracePtr(), Self, &Args, nullptr);
            if (auto* cc = CallContext()) {
                FILESTORE_TRACK(
                    TxPrepare,
                    cc,
                    TTx::Name);
            }

            LOG_TRACE(ctx, T::LogComponent,
                "[%lu] PrepareTx %s (gen: %u, step: %u)",
                Self->TabletID(),
                TTx::Name,
                Generation,
                Step);

            if (!TTx::PrepareTx(*Self, ctx, tx, Args)) {
                Args.Clear();
                return false;
            }

            tx.DB.NoMoreReadsForTx();

            // FILESTORE_TRACE_EVENT(ctx, "ExecuteTx", TracePtr(), Self, &Args, nullptr);
            if (auto* cc = CallContext()) {
                FILESTORE_TRACK(
                    TxExecute,
                    cc,
                    TTx::Name);
            }

            LOG_TRACE(ctx, T::LogComponent,
                "[%lu] ExecuteTx %s (gen: %u, step: %u)",
                Self->TabletID(),
                TTx::Name,
                Generation,
                Step);

            TTx::ExecuteTx(*Self, ctx, tx, Args);
            return true;
        }

        void Complete(const NActors::TActorContext& ctx) override
        {
            // FILESTORE_TRACE_EVENT(ctx, "CompleteTx", TracePtr(), Self, &Args, nullptr);
            if (auto* cc = CallContext()) {
                FILESTORE_TRACK(
                    TxComplete,
                    cc,
                    TTx::Name);
            }

            LOG_TRACE(ctx, T::LogComponent,
                "[%lu] CompleteTx %s (gen: %u, step: %u)",
                Self->TabletID(),
                TTx::Name,
                Generation,
                Step);

            TTx::CompleteTx(*Self, ctx, Args);
        }

    private:
        NWilson::TTraceId* TracePtr() const
        {
            return Args.RequestInfo ? &Args.RequestInfo->TraceId : nullptr;
        }

        TCallContext* CallContext() const
        {
            return Args.RequestInfo
                ? Args.RequestInfo->CallContext.Get()
                : nullptr;
        }

        ui64 RequestId() const
        {
            if (auto t = TracePtr()) {
                return GetRequestId(*t);
            }
            return 0;
        }
    };

    template <typename TTx, typename ...TArgs>
    std::unique_ptr<TTransaction<TTx>> CreateTx(TArgs&& ...args)
    {
        return std::make_unique<TTransaction<TTx>>(
            static_cast<T*>(this),
            std::forward<TArgs>(args)...);
    }

    template <typename TTx, typename ...TArgs>
    void ExecuteTx(const NActors::TActorContext& ctx, TArgs&& ...args)
    {
        auto tx = CreateTx<TTx>(std::forward<TArgs>(args)...);
        tx->Init(ctx);
        TTabletExecutedFlat::Execute(tx.release(), ctx);
    }

    void ExecuteTx(
        const NActors::TActorContext& ctx,
        std::unique_ptr<ITransactionBase> tx)
    {
        tx->Init(ctx);
        TTabletExecutedFlat::Execute(tx.release(), ctx);
    }
};

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_IMPLEMENT_TRANSACTION(name, ns)                              \
    struct T##name                                                             \
    {                                                                          \
        using TArgs = ns::T##name;                                             \
                                                                               \
        static constexpr const char* Name = #name;                             \
        static constexpr NKikimr::TTxType TxType = TCounters::TX_##name;       \
                                                                               \
        template <typename T, typename ...Args>                                \
        static bool PrepareTx(T& target, Args&& ...args)                       \
        {                                                                      \
            return target.PrepareTx_##name(std::forward<Args>(args)...);       \
        }                                                                      \
                                                                               \
        template <typename T, typename ...Args>                                \
        static void ExecuteTx(T& target, Args&& ...args)                       \
        {                                                                      \
            target.ExecuteTx_##name(std::forward<Args>(args)...);              \
        }                                                                      \
                                                                               \
        template <typename T, typename ...Args>                                \
        static void CompleteTx(T& target, Args&& ...args)                      \
        {                                                                      \
            target.CompleteTx_##name(std::forward<Args>(args)...);             \
        }                                                                      \
    };                                                                         \
                                                                               \
    bool PrepareTx_##name(                                                     \
        const NActors::TActorContext& ctx,                                     \
        NKikimr::NTabletFlatExecutor::TTransactionContext& tx,                 \
        ns::T##name& args);                                                    \
                                                                               \
    void ExecuteTx_##name(                                                     \
        const NActors::TActorContext& ctx,                                     \
        NKikimr::NTabletFlatExecutor::TTransactionContext& tx,                 \
        ns::T##name& args);                                                    \
                                                                               \
    void CompleteTx_##name(                                                    \
        const NActors::TActorContext& ctx,                                     \
        ns::T##name& args);                                                    \
// FILESTORE_IMPLEMENT_TRANSACTION

}   // namespace NCloud::NFileStore::NStorage
