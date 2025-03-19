#include "async_delivery.h"
#include "addr.h"

#include <search/session/logger/logger.h>

#include <library/cpp/messagebus/scheduler/scheduler.h>
#include <library/cpp/deprecated/atomic/atomic_ops.h>
#include <kernel/common_server/library/unistat/cache.h>

namespace {
    class TEventProcessor: public IObjectInQueue {
    private:
        NNeh::IMultiClient::TEvent Event;

    public:
        TEventProcessor(const NNeh::IMultiClient::TEvent& ev)
            : Event(ev)
        {
        }

        void Process(void*) override {
            THolder<TEventProcessor> this_(this);
            CHECK_WITH_LOG(Event.UserData);
            TAddrDelivery* sm = (TAddrDelivery*)Event.UserData;
            sm->OnEvent(Event);
        }
    };
}

class TAsyncDeliveryResources::TMetricsWatcher: public IObjectInQueue {
private:
    const TAsyncDeliveryResources* Owner;
public:
    TMetricsWatcher(const TAsyncDeliveryResources* owner)
        : Owner(owner)
    {

    }

    void Process(void*) override {
        while (Owner->IsActive()) {
            TCSSignals::SignalLastX("async_delivery", "requests", Owner->RequestsProcessingQueue.Size());
            Sleep(TDuration::Seconds(5));
        }
    }
};

class TAsyncDeliveryResources::TMCWaiter: public IObjectInQueue {
private:
    TAsyncDeliveryResources* AD;
    NNeh::IMultiClient& MultiClient;
    TThreadPool& RequestsProcessing;

public:
    ~TMCWaiter() {
        AD->RemoveWaiter();
    }

    TMCWaiter(TAsyncDeliveryResources* asyncDelivery, NNeh::IMultiClient& multiClient)
        : AD(asyncDelivery)
        , MultiClient(multiClient)
        , RequestsProcessing(asyncDelivery->RequestsProcessingQueue)
    {
        AD->AddWaiter();
    }

    void Process(void*) override {
        while (AD->IsActive()) {
            NNeh::IMultiClient::TEvent ev;
            if (MultiClient.Wait(ev, Now() + TDuration::Seconds(5))) {
                if (ev.Type == NNeh::IMultiClient::TEvent::Timeout) {
                    ev.Hndl->Cancel();
                }
                CHECK_WITH_LOG(RequestsProcessing.Add(new TEventProcessor(ev)));
            } else {
                NanoSleep(10);
            }
        }
    }
};

class TAsyncDelivery::TSourcesSwitcher
    : public NBus::NPrivate::IScheduleItem
    , public TGuard<IShardDelivery>
{
private:
    IShardDelivery::TPtr Source;
    TAsyncDelivery& AD;

public:
    TSourcesSwitcher(IShardDelivery::TPtr source, TAsyncDelivery& ad, const TInstant switchDeadline)
        : NBus::NPrivate::IScheduleItem(switchDeadline)
        , TGuard<IShardDelivery>(*source)
        , Source(source)
        , AD(ad)
    {
    }

    void Do() override {
        AD.AddRequest(Source);
    }
};

void TAsyncDelivery::AddRequest(IShardDelivery::TPtr source) {
    CHECK_WITH_LOG(source);
    TGuard<IShardDelivery> g(*source);
    if (InitReaskUnsafe(source.Get())) {
        CHECK_WITH_LOG(Resources->GetScheduler()) << "AsyncDelivery Not started";
        Resources->GetScheduler()->Schedule(new TSourcesSwitcher(source, *this, Now() + source->GetSwitchDuration()));
    }
}

bool TAsyncDelivery::InitReaskUnsafe(IShardDelivery* source) {
    CHECK_WITH_LOG(source);
    THolder<IAddrDelivery> addrDelivery(source->Next());
    if (!!addrDelivery) {
        const NNeh::TMessage& message = addrDelivery->GetMessage();
        const TInstant deadline = addrDelivery->GetDeadline();
        NNeh::IMultiClient::TRequest request(message, deadline, addrDelivery.Release());
        try {
            Resources->GetMultiClient(AtomicIncrement(ClientIdx))->Request(request);
        } catch (...) {
            auto evLogger = source->GetOwner()->GetReportBuilder().GetEventLogger();
            if (!!evLogger) {
                evLogger->LogEvent(NEvClass::TStageMessage(source->GetShardId(), message.Addr + message.Data, "MultiClient FAIL: " + CurrentExceptionMessage()));
            } else {
                ERROR_LOG << CurrentExceptionMessage() << Endl;
            }
        }
        return true;
    }
    return false;
}

TAsyncDeliveryResources* TAsyncDeliveryResources::GetGlobalInstance() {
    return SingletonWithPriority<TAsyncDeliveryResources, 128000>();
}

TAsyncDelivery::TAsyncDelivery() {
    if (TAsyncDeliveryResources::GetGlobalUsageFlag()) {
        CHECK_WITH_LOG(TAsyncDeliveryResources::GetGlobalInstance()->IsActive());
        Resources = TAsyncDeliveryResources::GetGlobalInstance();
    }
}

TAsyncDelivery::~TAsyncDelivery() {
    Y_ASSERT(AtomicGet(IsActiveFlag) == 0);
    if (AtomicGet(IsActiveFlag)) {
        Stop();
    }
}

void TAsyncDelivery::Start(const ui32 threadsMC, const ui32 threadsRequests) {
    AtomicSet(IsActiveFlag, 1);
    IsInternalResources = !Resources;
    if (IsInternalResources) {
        ResourcesStorage = MakeHolder<TAsyncDeliveryResources>();
        Resources = ResourcesStorage.Get();
        Resources->Start(threadsMC, threadsRequests);
    }
}

void TAsyncDelivery::Stop() {
    AtomicSet(IsActiveFlag, 0);
    if (IsInternalResources) {
        Resources->Stop();
    }
}

bool TAsyncDelivery::InitReask(IShardDelivery* source) {
    CHECK_WITH_LOG(source);
    TGuard<IShardDelivery> g(*source);
    return InitReaskUnsafe(source);
}

void TAsyncDelivery::Send(TAsyncTask* message) noexcept {
    TGuard<TAsyncTask> g(*message);
    for (auto&& source : message->GetShards()) {
        for (ui32 att = 0; att < source->GetParallelRequestsCountOnStart(); ++att) {
            AddRequest(source);
        }
    }
}

namespace {
    bool GlobalAsyncDeliveryResourcesUsageFlag = false;
}

void TAsyncDeliveryResources::InitGlobalResources(const ui32 MCThreadsCount, const ui32 RequestsProcessingThreadsCount) {
    GlobalAsyncDeliveryResourcesUsageFlag = true;
    GetGlobalInstance()->Start(MCThreadsCount, RequestsProcessingThreadsCount, true);
}

void TAsyncDeliveryResources::InitGlobalResources(const TConfig& config) {
    GlobalAsyncDeliveryResourcesUsageFlag = true;
    GetGlobalInstance()->Start(config.GetMultiClientsCount(), config.GetRequestsProcessingThreads(), true);
}

void TAsyncDeliveryResources::AddWaiter() {
    AtomicIncrement(WaitersCounter);
}

void TAsyncDeliveryResources::RemoveWaiter() {
    AtomicDecrement(WaitersCounter);
}

TAsyncDeliveryResources::~TAsyncDeliveryResources() {
    if (IsGlobalResources) {
        Stop();
    } else {
        CHECK_WITH_LOG(!IsActive());
    }
}

bool TAsyncDeliveryResources::GetGlobalUsageFlag() {
    return GlobalAsyncDeliveryResourcesUsageFlag;
}

void TAsyncDeliveryResources::Start(const ui32 MCThreadsCount, const ui32 RequestsProcessingThreadsCount, const bool isGlobalResources) {
    if (!AtomicCas(&ActiveFlag, 1, 0)) {
        CHECK_WITH_LOG(IsGlobalResources);
        return;
    }
    IsGlobalResources = isGlobalResources;
    if (IsGlobalResources) {
        MetricsWatcher.Start(1);
        CHECK_WITH_LOG(MetricsWatcher.AddAndOwn(MakeHolder<TMetricsWatcher>(this)));
    }
    RequestsProcessingQueue.Start(RequestsProcessingThreadsCount);
    MCQueue.Start(MCThreadsCount);
    for (ui32 i = 0; i < MCThreadsCount; ++i) {
        MultiClient.push_back(NNeh::CreateMultiClient());
        CHECK_WITH_LOG(MCQueue.AddAndOwn(MakeHolder<TMCWaiter>(this, *MultiClient.back())));
    }
    Scheduler = MakeHolder<NBus::NPrivate::TScheduler>();
}

void TAsyncDeliveryResources::Stop() {
    if (!AtomicCas(&ActiveFlag, 0, 1)) {
        CHECK_WITH_LOG(IsGlobalResources);
        return;
    }
    if (IsGlobalResources) {
        MetricsWatcher.Stop();
    }
    while (AtomicGet(WaitersCounter)) {
        for (auto&& i : MultiClient) {
            i->Interrupt();
        }
        Sleep(TDuration::MilliSeconds(100));
    }
    MCQueue.Stop();
    Scheduler->Stop();
    Scheduler.Destroy();
    RequestsProcessingQueue.Stop();
}
