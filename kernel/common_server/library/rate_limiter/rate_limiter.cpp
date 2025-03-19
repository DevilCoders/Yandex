#include "rate_limiter.h"

void TRequestRateLimiterConfig::Init(const TYandexConfig::Section* section) {
    MaxWeightPerSecond = section->GetDirectives().Value<size_t>("MaxWeightPerSecond", MaxWeightPerSecond);
    MaxRequestsPerSecond = section->GetDirectives().Value<size_t>("MaxRequestsPerSecond", MaxRequestsPerSecond);
    MaxWaitDuration = section->GetDirectives().Value<TDuration>("MaxWaitDuration", MaxWaitDuration);
}

void TRequestRateLimiterConfig::ToString(IOutputStream& os) const {
    os << "MaxWeightPerSecond: " << MaxWeightPerSecond << Endl;
    os << "MaxRequestsPerSecond: " << MaxRequestsPerSecond << Endl;
    os << "MaxWaitDuration: " << MaxWaitDuration << Endl;
}

TRequestRateLimiter::TRequestRateLimiter(NNeh::THttpClient& agent, const TRequestRateLimiterConfig& config)
    : TBase(config)
{
    QueueWorker.Reset(new TQueueWorker(JournalMutex, ExecutionQueue, agent));

    WorkerPool.Start(1);
    WorkerPool.SafeAdd(QueueWorker.Get());
}

TRequestRateLimiter::~TRequestRateLimiter() {
    {
        TGuard g(JournalMutex);
        QueueWorker->Shutdown();
    }
    WorkerPool.Stop();
}

NUtil::THttpReply TRequestRateLimiter::CreateErrorReply() const {
    NUtil::THttpReply reply;
    reply.SetCode(418);
    return reply;
}

NThreading::TFuture<NUtil::THttpReply> TRequestRateLimiter::Send(const NNeh::THttpRequest& request, const TDuration timeout, const size_t requestWeight) const {
    TInstant currentInstant = Now();
    TInstant startInstant = currentInstant;

    TGuard g(JournalMutex);
    TVector<TLogEntry> removedLogEntries;
    while (!Journal.empty() && (CurrentJournalWeight + requestWeight > MaxWeightPerSecond || Journal.size() + 1 > MaxRequestsPerSecond)) {
        auto& itemForDeletion = Journal.front();
        CurrentJournalWeight -= itemForDeletion.GetWeight();
        startInstant = Max(startInstant, itemForDeletion.GetRequestInstant() + TDuration::Seconds(1));
        removedLogEntries.emplace_back(itemForDeletion);
        Journal.pop_front();
    }

    if (startInstant - currentInstant > MaxWaitDuration) {
        for (auto&& entryIt = removedLogEntries.rbegin(); entryIt != removedLogEntries.rend(); ++entryIt) {
            Journal.emplace_front(std::move(*entryIt));
        }
        return NThreading::MakeFuture(CreateErrorReply());
    }

    Journal.emplace_back(requestWeight, startInstant);
    CurrentJournalWeight += requestWeight;

    auto promise = NThreading::NewPromise<NUtil::THttpReply>();
    ExecutionQueue.emplace(startInstant, request, timeout, promise);
    QueueWorker->NotifyNewRequest();
    return promise.GetFuture();
}

TMap<TString, NUtil::THttpReply> TRequestRateLimiter::SendPack(const TMap<TString, NNeh::THttpRequest>& requests, const TDuration timeout, const size_t requestWeight) const {
    TMap<TString, NThreading::TFuture<NUtil::THttpReply>> replyFutures;
    for (auto&& [id, request] : requests) {
        replyFutures.emplace(id, Send(request, timeout, requestWeight));
    }
    TMap<TString, NUtil::THttpReply> result;
    for (auto&& [id, replyFuture] : replyFutures) {
        result.emplace(id, replyFuture.GetValueSync());
    }
    return result;
}

void TRequestRateLimiter::TQueuedRequest::SetPromise(const NUtil::THttpReply& reply) const {
    ReplyPromise.SetValue(reply);
}

void TRequestRateLimiter::TQueueWorker::Process(void* /*threadSpecificResource*/) {
    JournalMutex.Acquire();
    while (Active) {
        NewQueueEntries.Wait(JournalMutex);
        auto now = Now();
        while (!ExecutionQueue.empty()) {
            TQueuedRequest task = ExecutionQueue.front();
            ExecutionQueue.pop();
            {
                JournalMutex.Release();
                PerformRequestUnsafe(task, now);
                JournalMutex.Acquire();
            }
            now = Now();
        }
    }
    JournalMutex.Release();
}

void TRequestRateLimiter::TQueueWorker::NotifyNewRequest() {
    NewQueueEntries.Signal();
}

void TRequestRateLimiter::TQueueWorker::Shutdown() {
    Active = false;
    NewQueueEntries.Signal();
}

void TRequestRateLimiter::TQueueWorker::PerformRequestUnsafe(TQueuedRequest& request, const TInstant now) {
    if (now < request.GetStartInstant()) {
        Sleep(request.GetStartInstant() - now);
    }
    Agent.SendAsync(request.GetRequest(), Now() + request.GetTimeout()).Apply([request](const NThreading::TFuture<NUtil::THttpReply>& r) {
        const auto& response = r.GetValue();
        request.SetPromise(response);
    });
}
