#pragma once

#include "addr.h"
#include "message.h"
#include "shard.h"

#include <library/cpp/neh/multiclient.h>

#include <util/thread/pool.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/yconf/conf.h>

namespace NBus {
    namespace NPrivate {
        class TScheduler;
    }
}

class TAsyncDeliveryResources {
private:
    TVector<NNeh::TMultiClientPtr> MultiClient;
    TThreadPool MCQueue;
    TThreadPool RequestsProcessingQueue;
    THolder<NBus::NPrivate::TScheduler> Scheduler;
    TAtomic WaitersCounter = 0;
    TAtomic ActiveFlag = 0;
    bool IsGlobalResources = false;
    TThreadPool MetricsWatcher;
    class TMCWaiter;
    class TMetricsWatcher;
public:

    NNeh::TMultiClientPtr& GetMultiClient(const i64 clientIdx) {
        return MultiClient[clientIdx % MultiClient.size()];
    }

    NBus::NPrivate::TScheduler* GetScheduler() {
        return Scheduler.Get();
    }

    class TConfig {
    private:
        RTLINE_READONLY_ACCEPTOR(Enabled, bool, false);
        RTLINE_READONLY_ACCEPTOR(MultiClientsCount, ui32, 8);
        RTLINE_READONLY_ACCEPTOR(RequestsProcessingThreads, ui32, 32);
    public:
        void Init(const TYandexConfig::Section* section) {
            CHECK_WITH_LOG(section);
            const auto& d = section->GetDirectives();
            Enabled = d.Value("Enabled", Enabled);
            RequestsProcessingThreads = d.Value("RequestsProcessingThreads", RequestsProcessingThreads);
            MultiClientsCount = d.Value("MultiClientsCount", MultiClientsCount);
        }

        void ToString(IOutputStream& os) const {
            os << "Enabled: " << Enabled << Endl;
            os << "MultiClientsCount: " << MultiClientsCount << Endl;
            os << "RequestsProcessingThreads: " << RequestsProcessingThreads << Endl;
        }
    };

    TAsyncDeliveryResources() = default;
    ~TAsyncDeliveryResources();

    static bool GetGlobalUsageFlag();
    static void InitGlobalResources(const ui32 MCThreadsCount, const ui32 RequestsProcessingThreadsCount);
    static void InitGlobalResources(const TConfig& config);
    static TAsyncDeliveryResources* GetGlobalInstance();

    bool IsActive() const {
        return AtomicGet(ActiveFlag);
    }

    void AddWaiter();
    void RemoveWaiter();

    void Start(const ui32 MCThreadsCount, const ui32 RequestsProcessingThreadsCount, const bool isGlobal = false);
    void Stop();

};

class TAsyncDelivery : public IAsyncDelivery, public TNonCopyable {
private:
    bool IsInternalResources = true;
    TAtomic IsActiveFlag = 0;
    TAtomic ClientIdx = 0;
    TAsyncDeliveryResources* Resources = nullptr;
    THolder<TAsyncDeliveryResources> ResourcesStorage;

private:
    void AddWaiter() {
        Resources->AddWaiter();
    }

    void RemoveWaiter() {
        Resources->RemoveWaiter();
    }

private:
    class TSourcesSwitcher;

private:
    void AddRequest(IShardDelivery::TPtr source);
    bool InitReaskUnsafe(IShardDelivery* source);

public:
    using TPtr = TAtomicSharedPtr<TAsyncDelivery>;

public:
    TAsyncDelivery();
    ~TAsyncDelivery();

    bool IsActive() const {
        return AtomicGet(IsActiveFlag);
    }

    void Start(const ui32 threadsMC, const ui32 threadsRequests);
    void Stop();

    bool InitReask(IShardDelivery* source) override;
    void Send(TAsyncTask* message) noexcept;
};
