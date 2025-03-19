#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/network/http_request.h>
#include <kernel/common_server/util/network/neh.h>
#include <kernel/common_server/util/network/neh_request.h>
#include <kernel/common_server/library/async_proxy/async_delivery.h>

#include <library/cpp/yconf/conf.h>
#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/future/subscription/wait_ut_common.h>

#include <util/datetime/base.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/generic/queue.h>
#include <util/generic/deque.h>

class TRequestRateLimiterConfig {
    RTLINE_PROTECT_ACCEPTOR(TRequestRateLimiterConfig, MaxRequestsPerSecond, size_t, Max<size_t>());
    RTLINE_PROTECT_ACCEPTOR(TRequestRateLimiterConfig, MaxWeightPerSecond, size_t, Max<size_t>());
    RTLINE_PROTECT_ACCEPTOR(TRequestRateLimiterConfig, MaxWaitDuration, TDuration, TDuration::Max());

public:
    void Init(const TYandexConfig::Section* section);
    void ToString(IOutputStream& os) const;
};

class TRequestRateLimiter : public TRequestRateLimiterConfig {
private:
    using TBase = TRequestRateLimiterConfig;
    using TBase::TBase;

public:
    TRequestRateLimiter(NNeh::THttpClient& agent, const TRequestRateLimiterConfig& config = TRequestRateLimiterConfig());
    ~TRequestRateLimiter();

    NThreading::TFuture<NUtil::THttpReply> Send(const NNeh::THttpRequest& request, const TDuration timeout, const size_t requestWeight = 1) const;
    TMap<TString, NUtil::THttpReply> SendPack(const TMap<TString, NNeh::THttpRequest>& requests, const TDuration timeout, const size_t requestWeight = 1) const;
    NUtil::THttpReply CreateErrorReply() const;

private:
    class TLogEntry {
        RTLINE_READONLY_ACCEPTOR(Weight, size_t, 0);
        RTLINE_READONLY_ACCEPTOR_DEF(RequestInstant, TInstant);

    public:
        TLogEntry(const size_t weight, const TInstant requestInstant)
            : Weight(weight)
            , RequestInstant(requestInstant)
        {
        }
    };

    class TQueuedRequest {
        RTLINE_READONLY_ACCEPTOR_DEF(StartInstant, TInstant);
        RTLINE_READONLY_ACCEPTOR_DEF(Request, NNeh::THttpRequest);
        RTLINE_READONLY_ACCEPTOR_DEF(Timeout, TDuration);

        mutable NThreading::TPromise<NUtil::THttpReply> ReplyPromise;

    public:
        TQueuedRequest(const TInstant startInstant, const NNeh::THttpRequest& request, const TDuration timeout, const NThreading::TPromise<NUtil::THttpReply>& replyPromise)
            : StartInstant(startInstant)
            , Request(request)
            , Timeout(timeout)
            , ReplyPromise(replyPromise)
        {
        }

        void SetPromise(const NUtil::THttpReply& reply) const;
    };

    class TQueueWorker : public IObjectInQueue {
    public:
        TQueueWorker(TMutex& journalMutex, TQueue<TQueuedRequest>& executionQueue, NNeh::THttpClient& agent)
            : JournalMutex(journalMutex)
            , ExecutionQueue(executionQueue)
            , Agent(agent)
        {
        }

        virtual void Process(void* /*threadSpecificResource*/) override;
        void NotifyNewRequest();
        void Shutdown();

    private:
        TMutex& JournalMutex;
        TQueue<TQueuedRequest>& ExecutionQueue;
        NNeh::THttpClient& Agent;
        TCondVar NewQueueEntries;
        bool Active = true;

        void PerformRequestUnsafe(TQueuedRequest& request, const TInstant now);
    };

    TThreadPool WorkerPool;
    mutable THolder<TQueueWorker> QueueWorker;

    TMutex JournalMutex;
    mutable TDeque<TLogEntry> Journal;
    mutable TQueue<TQueuedRequest> ExecutionQueue;
    mutable size_t CurrentJournalWeight = 0;
};
