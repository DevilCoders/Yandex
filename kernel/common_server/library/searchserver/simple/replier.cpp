#include "replier.h"
#include "exception.h"
#include <kernel/common_server/library/unistat/signals.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/util/auto_actualization.h>

IHttpReplier::IHttpReplier(IReplyContext::TPtr context, const THttpStatusManagerConfig* config)
    : HttpStatusConfig(config ? *config : THttpStatusManagerConfig())
    , Context(context)
{
}

void IHttpReplier::Process(void* /*ThreadSpecificResource*/) {
    THolder<IHttpReplier> this_(this);
    SearchAndReply();
}

void IHttpReplier::Reply() {
    THolder<IHttpReplier> cleanup(this);
    IThreadPool* queue = DoSelectHandler();
    if (!queue) {
        SearchAndReply();
        return;
    }

    if (queue->Add(cleanup.Get())) {
        Y_UNUSED(cleanup.Release());
    } else {
        OnQueueFailure();
    }
}

void IHttpReplier::OnQueueFailure() {
    Y_ASSERT(Context);
    MakeErrorPage(HTTP_SERVICE_UNAVAILABLE, "Cannot queue request");
}

void IHttpReplier::MakeErrorPage(ui32 code, const TString& error) {
    ::MakeErrorPage(Context, code, error);
}

namespace {
    TRWMutex Mutex;

    class TPoolHandlerKey {
    private:
        CSA_DEFAULT(TPoolHandlerKey, TString, Handler);
        CSA_DEFAULT(TPoolHandlerKey, TString, PoolId);
    public:
        TPoolHandlerKey(const TString& handler, const TString& poolId)
            : Handler(handler)
            , PoolId(poolId)
        {

        }

        bool operator<(const TPoolHandlerKey& item) const {
            return std::tie(Handler, PoolId) < std::tie(item.Handler, item.PoolId);
        }
    };

    TMap<TPoolHandlerKey, TAtomic> ThreadsUsage;
    class TThreadsUsageSignals: public IAutoActualization {
    private:
        using TBase = IAutoActualization;
    public:
        TThreadsUsageSignals()
            : TBase("threads_usage_by_handlers_watcher", TDuration::Seconds(1))
        {
            TBase::Start();
        }

        ~TThreadsUsageSignals() {
            TBase::Stop();
        }

        virtual bool Refresh() override {
            TMap<TPoolHandlerKey, TAtomic> threadsUsage;
            {
                TReadGuard rg(Mutex);
                threadsUsage = ThreadsUsage;
            }
            TMap<TString, ui32> poolUsage;
            for (auto&& i : threadsUsage) {
                TCSSignals::LTSignal("thread_pool_usage", AtomicGet(i.second))("code", "in_progress")("handler", i.first.GetHandler())("pool_id", i.first.GetPoolId());
                poolUsage[i.first.GetPoolId()] += AtomicGet(i.second);
            }
            for (auto&& i : poolUsage) {
                TCSSignals::LTSignal("thread_pool_usage", i.second)("code", "usage_variants_count")("pool_id", i.first);
            }
            return true;
        }

    };
    class TThreadsUsageGuard {
    private:
        const TPoolHandlerKey Key;
    public:
        TThreadsUsageGuard(const TString& uri, const TString& poolId)
            : Key(uri, poolId)
        {
            TCSSignals::Signal("thread_pool_usage")("handler", Key.GetHandler())("code", "thread_usage_started")("pool_id", Key.GetPoolId());
            TReadGuard rg(Mutex);
            auto it = ThreadsUsage.find(Key);
            if (it == ThreadsUsage.end()) {
                rg.Release();
                TWriteGuard wg(Mutex);
                auto itW = ThreadsUsage.find(Key);
                if (itW == ThreadsUsage.end()) {
                    itW = ThreadsUsage.emplace(Key, TAtomic()).first;
                }
                AtomicIncrement(itW->second);
            } else {
                AtomicIncrement(it->second);
            }
        }

        ~TThreadsUsageGuard() {
            TCSSignals::Signal("thread_pool_usage")("handler", Key.GetHandler())("code", "thread_usage_finished")("pool_id", Key.GetPoolId());
            TReadGuard rg(Mutex);
            auto it = ThreadsUsage.find(Key);
            CHECK_WITH_LOG(it != ThreadsUsage.end());
            AtomicDecrement(it->second);
        }
    };
    TThreadsUsageSignals ThreadsWatcher;
}

void IHttpReplier::SearchAndReply() {
    TThreadsUsageGuard threadsGuard(Context->GetUri(), Context->GetThreadPoolId());
    TCSSignals::HSignal("http.queue_waiting", NRTProcHistogramSignals::IntervalsRTLineReply)("handler", Context->GetUri())((Now() - Context->GetRequestStartTime()).MilliSeconds());
    try {
        auto deadlineCheckResult = Context->DeadlineCorrection(GetDefaultTimeout(), GetDefaultKffWaitingAvailable());
        TCSSignals::Signal("http_handlers")("metric", "deadline_status")("handler", Context->GetUri())("status", deadlineCheckResult);
        if (deadlineCheckResult == EDeadlineCorrectionResult::dcrRequestExpired) {
            try {
                OnRequestExpired();
            } catch (...) {
                throw TSearchException(HttpStatusConfig.TimeoutStatus, yxTIMEOUT) << "request timeout: " << CurrentExceptionMessage();
            }
        } else if (deadlineCheckResult == EDeadlineCorrectionResult::dcrIncorrectDeadline) {
            throw TSearchException(HttpStatusConfig.SyntaxErrorStatus, yxSYNTAX_ERROR) << "incorrect &timeout=<value>";
        }
        DoSearchAndReply();
    } catch (const TSearchException& se) {
        MakeErrorPage(se.GetHttpCode(), se.what());
    } catch (const yexception& e) {
        MakeErrorPage(HttpStatusConfig.UnknownErrorStatus, e.what());
        WARNING_LOG << "Unexpected exception in SearchAndReply [" << Context->GetRequestId() << "]: " << e.what() << Endl;
    }
}

