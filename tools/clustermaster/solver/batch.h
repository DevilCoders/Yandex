#pragma once

#include "request.h"

#include <tools/clustermaster/communism/core/core.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/intrlist.h>
#include <util/generic/string.h>
#include <util/network/pollerimpl.h>
#include <util/system/spinlock.h>
#include <util/thread/pool.h>

using namespace NCommunism;

struct TBatch;

struct TBatchUpdater {
    TBatch* const Batch;

    TRequestsHandle Requests;

    TBatchUpdater(TBatch* batch, TRequestsHandle requests)
        : Batch(batch)
        , Requests(requests)
    {
    }

    void operator()(TAutoPtr<NCore::TPackedMessage> what) const;
};

struct TAtomicRefOps {
    static void Acquire(TAtomic* counter) noexcept {
        AtomicIncrement(*counter);
    }

    static void Release(TAtomic* counter) noexcept {
        AtomicDecrement(*counter);
    }
};

struct TBatchProcessor: IObjectInQueue {
    TGuard<TAtomic, TAtomicRefOps> Guard;

    TBatch* const Batch;

    const bool Recv;
    const bool Send;

    NGlobal::TPoller* const Poller;

    const TInstant Now;

    TBatchProcessor(TAtomic* counter, TBatch* batch, bool recv, bool send, NGlobal::TPoller* poller, TInstant now) noexcept
        : Guard(counter)
        , Batch(batch)
        , Recv(recv)
        , Send(send)
        , Poller(poller)
        , Now(now)
    {
    }

    void Process(void*) override;
};

struct TBatch
    : THashTable<TRequest, NCore::TKey, THash<NCore::TKey>, TRequest::TExtractKey, TEqualTo<NCore::TKey>, TAllocator>
    , NCore::TTransceiver<TBatchUpdater>
    , TPollerCookie
    , TIntrusiveListItem<TBatch>
{
    enum {
        MaxBytesPerRecvSend = 16384,
        DefaultHashtableSize = 512
    };

    const TString Id;
    TInstant LastHeartbeat;

    TBatch(TAutoPtr<TStreamSocket> socket, const ISockAddr& addr, TRequestsHandle requests)
        : THashTable<value_type, key_type, hasher, TRequest::TExtractKey, key_equal, TAllocator>(DefaultHashtableSize, hasher(), key_equal())
        , NCore::TTransceiver<TBatchUpdater>(TBatchUpdater(this, requests), socket, MaxBytesPerRecvSend)
        , Id(addr.ToString())
        , LastHeartbeat(TInstant::Now())
    {
    }

    void UpdatePoller(NGlobal::TPoller& poller) const noexcept {
        poller.Set(GetCookie(), *GetSocket(), CONT_POLL_READ | (HasDataToSend() ? CONT_POLL_WRITE : 0));
    }
};

typedef TIntrusiveListWithAutoDelete<TBatch, TDelete> TBatches;
