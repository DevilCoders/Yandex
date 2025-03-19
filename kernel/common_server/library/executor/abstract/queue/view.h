#pragma once
#include "abstract.h"
#include "common.h"
#include <util/system/mutex.h>
#include <util/thread/pool.h>
#include <kernel/common_server/util/lqueue.h>
#include <library/cpp/messagebus/scheduler/scheduler.h>
#include <kernel/common_server/library/unistat/signals.h>
#include <kernel/common_server/util/algorithm/container.h>

class TTasksActivityDurationSignal: public TUnistatSignal<double> {
public:
    TTasksActivityDurationSignal(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-task-duration", NRTProcHistogramSignals::IntervalsTasks) {

    }
};

class TTasksActivationDelaySignal: public TUnistatSignal<double> {
public:
    TTasksActivationDelaySignal(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-task-dequeue-delay",
            NRTProcHistogramSignals::IntervalsRTLineReply) {

    }
};

class TDistributedQueueSize: public TUnistatSignal<double> {
public:
    TDistributedQueueSize(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-full-size", EAggregationType::LastValue, "axxx") {

    }
};

class TUnknownTasksQueueSize: public TUnistatSignal<double> {
public:
    TUnknownTasksQueueSize(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-unknown-count", EAggregationType::LastValue, "ammm") {

    }
};

class TExternalLockTasksQueueSize: public TUnistatSignal<double> {
public:
    TExternalLockTasksQueueSize(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-external-count", EAggregationType::LastValue, "ammm") {

    }
};

class TLockedTasksQueueSize: public TUnistatSignal<double> {
public:
    TLockedTasksQueueSize(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-locked-count", EAggregationType::LastValue, "ammm") {

    }
};

class TRelockTasksQueueSize: public TUnistatSignal<double> {
public:
    TRelockTasksQueueSize(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-relock-count", EAggregationType::LastValue, "ammm") {

    }
};

class TScheduledTasksQueueSize: public TUnistatSignal<double> {
public:
    TScheduledTasksQueueSize(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-scheduled-count", EAggregationType::LastValue, "ammm") {

    }
};

class TActiveTasksCounter: public TUnistatSignal<double> {
public:
    TActiveTasksCounter(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-active-count", EAggregationType::LastValue, "ammm") {

    }
};

class TCallbacksTasksCounter: public TUnistatSignal<double> {
public:
    TCallbacksTasksCounter(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-callbacks-count", EAggregationType::LastValue, "ammm") {

    }
};

class TTasksOverflowTasksCounter: public TUnistatSignal<double> {
public:
    TTasksOverflowTasksCounter(const TString& nameQueue)
        : TUnistatSignal<double>(nameQueue + "-overflow-count", EAggregationType::LastValue, "axxx") {

    }
};


class IDistributedTaskContext;

class IQueueViewSelector {
protected:
    IDistributedTaskContext* Context = nullptr;
public:
    using TPtr = TAtomicSharedPtr<IQueueViewSelector>;
    using TFactory = NObjectFactory::TParametrizedObjectFactory<IQueueViewSelector, TString, IDistributedTaskContext*>;
    virtual ~IQueueViewSelector() = default;

    IQueueViewSelector(IDistributedTaskContext* context)
        : Context(context)
    {

    }

    virtual bool IsAvailable(const TString& taskId) const = 0;
};

class TNoTasksSelector: public IQueueViewSelector {
private:
    static TFactory::TRegistrator<TNoTasksSelector> Registrator;
public:

    using IQueueViewSelector::IQueueViewSelector;

    virtual bool IsAvailable(const TString& /*taskId*/) const {
        return false;
    }
};

class TAllTasksSelector: public IQueueViewSelector {
private:
    static TFactory::TRegistrator<TAllTasksSelector> Registrator;
public:

    using IQueueViewSelector::IQueueViewSelector;

    virtual bool IsAvailable(const TString& /*taskId*/) const {
        return true;
    }
};

class ITasksWaiter {
public:
    virtual ~ITasksWaiter() = default;
    virtual void Update() noexcept = 0;
    virtual bool RegisterWaiter(TTaskGuard::TPtr guard) noexcept = 0;
};

class IDistributedTasksQueueConfig;

class TDistributedQueueView: public IQueueActor, public IMessageProcessor {
private:
    TMutex Mutex;
    TMap<TString, TTaskGuard::TPtr> TasksInfo;
    TLQueue<TString> TasksUnknown;
    TLQueue<TString> TasksRelock;
    TLQueue<TString> TasksExternalLock;
    TLQueue<TString> TasksLocked;

    TAtomic ActiveTasksCounter = 0;

    TTasksActivityDurationSignal TasksActivityDurationSignal;
    TTasksActivationDelaySignal TasksActivationDelaySignal;
    TDistributedQueueSize QueueSizeSignal;
    TUnknownTasksQueueSize TasksUnknownSignal;
    TRelockTasksQueueSize TasksRelockSignal;
    TScheduledTasksQueueSize TasksScheduledSignal;
    TLockedTasksQueueSize TasksLockedSignal;
    TExternalLockTasksQueueSize TasksExternalLockSignal;
    TActiveTasksCounter TasksActiveSignal;
    TCallbacksTasksCounter TasksCallbacksSignal;
    TTasksOverflowTasksCounter TasksOverflowSignal;

    TMutex DequeueMutex;
    NBus::NPrivate::TScheduler Scheduler;
    IDistributedQueue::TPtr Queue;
    TThreadPool Watchers;
    TAtomic IsActiveFlag = 0;
    IQueueViewSelector::TPtr Selector;
    const ui64 LockedTasksLimit = 0;
    const TDuration ReadQueueTimeout = TDuration::Seconds(10);
    const ui32 ReadQueueAttempts = 16;
    const TDuration ReadQueueFailTimeout = TDuration::Max();
    const TDuration PingPeriod = TDuration::Seconds(1);
    ui32 ThreadsCount = 8;

    TRWMutex NodesMutex;
    TSet<TString> Nodes;
    TSet<TString> NewNodes;
    TSet<TString> RemovedNodes;

    TMutex TasksStartMutex;
    TSet<TString> TasksStart;

    class TDeferredTask;
    class TQueueWatcher;
    class TUnknownTasksWatcher;
    class TRelockTasksWatcher;
    class TExternalLocksTasksWatcher;
    class TTasksWaiter;

    THolder<ITasksWaiter> TasksWaiter;

    void AddExternalLocked(const TString& taskId);

    void AddRelock(const TString& taskId);

    bool AddLockedTask(TTaskGuard::TPtr guard, const bool force);

    bool AddExistanceTask(const TString& taskId);

    void RemoveTaskFromView(const TString& taskId);

    class TGuardTaskStart {
    private:
        const TString TaskId;
        TDistributedQueueView* View;
    public:
        TGuardTaskStart(const TString& taskId, TDistributedQueueView* view)
            : TaskId(taskId)
            , View(view)
        {
            TGuard<TMutex> g(View->TasksStartMutex);
            View->TasksStart.emplace(TaskId);
        }

        ~TGuardTaskStart() {
            TGuard<TMutex> g(View->TasksStartMutex);
            View->TasksStart.erase(TaskId);
        }

    };
public:
    TDuration GetReadQueueTimeout() const {
        return ReadQueueTimeout;
    }

    ui32 GetReadQueueAttempts() const {
        return ReadQueueAttempts;
    }

    TDuration GetReadQueueFailTimeout() const {
        return ReadQueueFailTimeout;
    }

    TDuration GetPingPeriod() const {
        return PingPeriod;
    }

    TDistributedQueueView& SetThreadsCount(const ui32 threadsCount) {
        CHECK_WITH_LOG(!IsActive());
        ThreadsCount = threadsCount;
        return *this;
    }

    IQueueViewSelector::TPtr GetSelector() const {
        return Selector;
    }

    bool Process(IMessage* message) override;

    virtual TString Name() const override {
        return Queue->GetName();
    }

    virtual ~TDistributedQueueView() {
        Scheduler.Stop();
    }

    bool IsActive() const {
        return AtomicGet(IsActiveFlag);
    }

    void Stop() {
        CHECK_WITH_LOG(AtomicCas(&IsActiveFlag, 0, 1));
        Watchers.Stop();
        for (auto&& task : TasksInfo) {
            WARNING_LOG << "Remaining task " << task.first << Endl;
        }
        TasksInfo.clear();
        UnregisterGlobalMessageProcessor(this);
    }

    void Start();

    TDistributedQueueView& SetSelector(IQueueViewSelector::TPtr selector) {
        Selector = selector;
        return *this;
    }

    TDistributedQueueView(IDistributedQueue::TPtr queue, const IDistributedTasksQueueConfig& config);

    bool Enqueue(const TTaskData& data, const bool rewrite = false);

    virtual void RemoveTask(const TString& taskId) override {
        INFO_LOG << "Remove task: " << taskId << Endl;
        {
            TWriteGuard wg(NodesMutex);
            RemovedNodes.emplace(taskId);
        }
        Queue->RemoveTask(taskId);
        RemoveTaskFromView(taskId);
    }

    TTaskGuard::TPtr Dequeue();

    ui32 GetSize(const bool noCache = true) const {
        if (noCache) {
            return Queue->GetSize();
        } else {
            TGuard<TMutex> g(Mutex);
            return TasksInfo.size();
        }
    }

    TVector<TString> GetAllNodes() const {
        TReadGuard g(NodesMutex);
        return MakeVector(Nodes);
    }

    TVector<TString> GetNodes(const bool noCache = true) const {
        if (noCache) {
            return Queue->GetAllTasks();
        } else {
            TVector<TString> result;
            TGuard<TMutex> g(Mutex);
            for (auto&& i : TasksInfo) {
                result.push_back(i.first);
            }
            return result;
        }
    }

    virtual NRTProc::TAbstractLock::TPtr WriteLock(const TString& key, const TDuration timeout) const override {
        return Queue->WriteLock(key, timeout);
    }

    virtual NRTProc::TAbstractLock::TPtr ReadLock(const TString& key, const TDuration timeout) const override {
        return Queue->ReadLock(key, timeout);
    }
};
