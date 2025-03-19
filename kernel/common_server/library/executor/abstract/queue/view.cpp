#include "view.h"

#include <kernel/daemon/messages.h>

#include <util/string/vector.h>
#include <util/string/join.h>
#include <util/system/guard.h>
#include <kernel/common_server/library/unistat/signals.h>
#include <kernel/common_server/util/algorithm/container.h>
#include <kernel/common_server/library/executor/abstract/queue.h>
#include <util/thread/pool.h>
#include <library/cpp/threading/future/async.h>
#include <kernel/common_server/library/unistat/cache.h>

class TDistributedQueueView::TDeferredTask: public NBus::NPrivate::IScheduleItem {
private:
    TDistributedQueueView* View;
    TString TaskId;
public:

    const TString& GetTaskId() const {
        return TaskId;
    }

    virtual ~TDeferredTask() = default;

    TDeferredTask(TDistributedQueueView* view, TTaskGuard::TPtr guard)
        : NBus::NPrivate::IScheduleItem(guard->GetData().GetStartInstant())
        , View(view)
        , TaskId(guard->GetTaskIdentifier())
    {
        CHECK_WITH_LOG(guard->IsLocked());
        guard->Unlock();
        CHECK_WITH_LOG(guard->GetData().HasStartInstant() || guard->GetData().IsCallback());
    }

    void Do() override {
        INFO_LOG << "deferred task " << TaskId << Endl;
        View->AddRelock(TaskId);
    }
};

class TDistributedQueueView::TQueueWatcher: public IObjectInQueue {
private:
    TDistributedQueueView* View;
    THolder<IThreadPool> ThreadsPool;
    TVector<TSet<TString>> NodesSet;
    TVector<TAtomicCounter> DeadlockFlags;
    TAtomic DeadAttempts = 0;
    ui32 CurrentUnlockedIdx = 0;
    TInstant LastRead = TInstant::Max();
public:

    TQueueWatcher(TDistributedQueueView* view)
        : View(view)
    {
        ThreadsPool = CreateThreadPool(View->GetReadQueueAttempts());
        NodesSet.resize(View->GetReadQueueAttempts(), TSet<TString>());
        DeadlockFlags.resize(View->GetReadQueueAttempts(), 0);
    }

    bool RecvQueueNodes(TSet<TString>& result) {
        while (View->IsActive()) {
            TCSSignals::SignalLastX("executor-view", "freshness", (Now() - LastRead).MilliSeconds());
            TCSSignals::SignalLastX("executor-view", "dead-attempts", AtomicGet(DeadAttempts));
            try {
                if (DeadlockFlags[CurrentUnlockedIdx].Val()) {
                    WARNING_LOG << "move view lock cursor: " << CurrentUnlockedIdx << " / " << AtomicGet(DeadAttempts) << " / " << View->GetReadQueueAttempts() << Endl;
                    CurrentUnlockedIdx = (CurrentUnlockedIdx + 1) % View->GetReadQueueAttempts();
                    Sleep(View->GetPingPeriod());
                    continue;
                }
                CHECK_WITH_LOG(Now() - LastRead < View->GetReadQueueFailTimeout()) << "Queue reading globally timeouted" << Endl;
                ui32 currentUnlockedIdx = CurrentUnlockedIdx;
                const auto action = [this, currentUnlockedIdx]() {
                    AtomicIncrement(DeadAttempts);
                    DeadlockFlags[currentUnlockedIdx].Inc();
                    NodesSet[currentUnlockedIdx] = MakeSet(View->Queue->GetAllTasks());
                    AtomicDecrement(DeadAttempts);
                    DeadlockFlags[currentUnlockedIdx].Dec();
                };
                if (!NThreading::Async(action, *ThreadsPool).Wait(Now() + View->GetReadQueueTimeout())) {
                    WARNING_LOG << "command is dead: View->Queue->GetAllTasks() for " << CurrentUnlockedIdx << " / " << AtomicGet(DeadAttempts) << Endl;
                    continue;
                }
                LastRead = Now();
                result = NodesSet[CurrentUnlockedIdx];
                return true;
            } catch (...) {
                ERROR_LOG << "Cannot take info about full tasks list: " << CurrentExceptionMessage() << Endl;
                Sleep(View->GetPingPeriod());
            }

        }
        return false;
    }

    void Process(void* /*threadSpecificResource*/) override {
        ui32 idx = 0;
        while (View->IsActive()) {
            TInstant start = Now();
            DEBUG_LOG << "iteration view: " << ++idx << Endl;
            TCSSignals::SignalSpec("executor-view", "hb", 1, EAggregationType::Sum, "dnnn");
            ui32 rejectedCount = 0;
            TSet<TString> nodes;
            if (!RecvQueueNodes(nodes)) {
                WARNING_LOG << "cannot read nodes from queue" << Endl;
                continue;
            }
            try {
                {
                    TWriteGuard wg(View->NodesMutex);
                    View->Nodes = nodes;
                    View->Nodes.insert(View->NewNodes.begin(), View->NewNodes.end());
                    View->NewNodes.clear();
                }
                View->QueueSizeSignal.Signal(nodes.size());
                {
                    View->TasksWaiter->Update();
                    TWriteGuard wg(View->NodesMutex);
                    for (auto&& i : nodes) {
                        if (View->RemovedNodes.contains(i)) {
                            continue;
                        }
                        if (!View->GetSelector() || View->GetSelector()->IsAvailable(i)) {
                            if (!View->AddExistanceTask(i)) {
                                ++ rejectedCount;
                            }
                        }
                    }
                    View->RemovedNodes.clear();
                }
            } catch (...) {
                ERROR_LOG << "Cannot take info about full tasks list: " << CurrentExceptionMessage() << Endl;
            }
            View->TasksOverflowSignal.Signal(rejectedCount);
            Sleep(View->GetPingPeriod() - (Now() - start));
        }
    }
};

class TDistributedQueueView::TTasksWaiter: public ITasksWaiter {
private:
    TDistributedQueueView* View = nullptr;
    class TBucketWait {
    private:
        TSet<TString> TasksWait;
        TDeferredTask Task;
        TAtomic TaskDoingCounter = 0;
    public:
        using TPtr = TAtomicSharedPtr<TBucketWait>;
        template <class TContainer>
        TBucketWait(const TContainer& c, const TDeferredTask& task)
            : TasksWait(c.begin(), c.end())
            , Task(task)
        {

        }

        bool Empty() const {
            return TasksWait.empty();
        }

        const TString& GetTaskId() const {
            return Task.GetTaskId();
        }

        void FinishDeadline() {
            DEBUG_LOG << Task.GetTaskId() << " DEADLINE (" << JoinSeq(",", TasksWait) << Endl;
            if (AtomicIncrement(TaskDoingCounter) == 1) {
                WARNING_LOG << "Waiter deadline: " << Task.GetTaskId() << Endl;
                Task.Do();
            }
        }

        bool RemoveWaitTask(const TString& id) {
            CHECK_WITH_LOG(TasksWait.erase(id) == 1);
            if (TasksWait.empty()) {
                WARNING_LOG << "Waiter start: " << id << " / " << Task.GetTaskId() << Endl;
                if (AtomicIncrement(TaskDoingCounter) == 1) {
                    Task.Do();
                }
                return true;
            }
            return false;
        }
    };

    TMap<TInstant, TBucketWait::TPtr> DeadlineBuckets;
    TMap<TString, TVector<TBucketWait::TPtr>> BucketsByTasks;
    TSet<TString> WaitTasks;
    TMutex Mutex;
public:
    TTasksWaiter(TDistributedQueueView* view)
        : View(view)
    {

    }

    bool RegisterWaiter(TTaskGuard::TPtr guard) noexcept override {
        {
            TGuard<TMutex> g(Mutex);
            if (WaitTasks.contains(guard->GetTaskIdentifier())) {
                DEBUG_LOG << "waiter registering duplication: " << JoinSeq(", ", guard->GetData().GetWaitTasks()) << " / " << guard->GetTaskIdentifier() << Endl;
                return true;
            }
        }
        DEBUG_LOG << "waiter registering: " << JoinSeq(", ", guard->GetData().GetWaitTasks()) << " / " << guard->GetTaskIdentifier() << Endl;
        CHECK_WITH_LOG(guard->HasData());
        const TSet<TString>& triggerTasks = guard->GetData().GetWaitTasks();
        TVector<TString> triggerTasksReal;
        TSet<TString> nodes;
        if (triggerTasks.empty() || guard->GetData().IsExpired()) {
            DEBUG_LOG << "waiter provided: " << JoinSeq(", ", guard->GetData().GetWaitTasks()) << " / " << guard->GetTaskIdentifier() << Endl;
            return false;
        }
        {
            TReadGuard rg(View->NodesMutex);
            nodes = View->Nodes;
        }
        auto itTrigger = triggerTasks.begin();
        auto itNodes = nodes.lower_bound(*itTrigger);
        for (; itNodes != nodes.end() && itTrigger != triggerTasks.end();) {
            if (*itNodes == *itTrigger) {
                triggerTasksReal.push_back(*itTrigger);
                ++itTrigger;
                ++itNodes;
            } else if (*itNodes < *itTrigger) {
                ++itNodes;
            } else {
                ++itTrigger;
            }
        }
        {
            if (triggerTasksReal.empty()) {
                DEBUG_LOG << JoinSeq(", ", nodes) << Endl;
                DEBUG_LOG << "waiter provided: " << JoinSeq(", ", guard->GetData().GetWaitTasks()) << " / " << guard->GetTaskIdentifier() << Endl;
                return false;
            } else {
                auto bucketWait = MakeAtomicShared<TBucketWait>(triggerTasksReal, TDeferredTask(View, guard));
                TGuard<TMutex> g(Mutex);
                WaitTasks.emplace(guard->GetTaskIdentifier());
                for (auto&& i : triggerTasksReal) {
                    BucketsByTasks[i].push_back(bucketWait);
                }
                if (guard->GetData().HasDeadline()) {
                    DeadlineBuckets.emplace(guard->GetData().GetDeadline(), bucketWait);
                }
            }
            DEBUG_LOG << "waiter registered: " << JoinSeq(", ", guard->GetData().GetWaitTasks()) << " / " << guard->GetTaskIdentifier() << Endl;
            TGuard<TMutex> g(Mutex);
            View->TasksCallbacksSignal.Signal(BucketsByTasks.size());
        }
        return true;
    }

    void Update() noexcept override {
        TGuard<TMutex> g(Mutex);
        TSet<TString> nodes;
        {
            TReadGuard rg(View->NodesMutex);
            nodes = View->Nodes;
        }
        for (auto it = DeadlineBuckets.begin(); it != DeadlineBuckets.end();) {
            if (it->first <= Now()) {
                it->second->FinishDeadline();
                it = DeadlineBuckets.erase(it);
            } else {
                break;
            }
        }
        if (BucketsByTasks.empty()) {
            return;
        }
        TVector<TString> triggerTasksReal;
        auto itTrigger = BucketsByTasks.begin();
        auto itNodes = nodes.lower_bound(BucketsByTasks.begin()->first);
        for (; itNodes != nodes.end() && itTrigger != BucketsByTasks.end();) {
            if (*itNodes == itTrigger->first) {
                ++itTrigger;
                ++itNodes;
            } else if (*itNodes < itTrigger->first) {
                ++itNodes;
            } else {
                for (auto&& i : itTrigger->second) {
                    if (i->RemoveWaitTask(itTrigger->first)) {
                        CHECK_WITH_LOG(WaitTasks.erase(i->GetTaskId()) == 1);
                    }
                }
                itTrigger = BucketsByTasks.erase(itTrigger);
            }
        }
        View->TasksCallbacksSignal.Signal(BucketsByTasks.size());
    }
};

class TDistributedQueueView::TUnknownTasksWatcher: public IObjectInQueue {
private:
    TDistributedQueueView* View;
public:

    TUnknownTasksWatcher(TDistributedQueueView* view)
        : View(view) {

    }

    void Process(void* /*threadSpecificResource*/) override {
        while (View->IsActive()) {
            TString taskId;
            while (View->TasksUnknown.Get(&taskId)) {
                try {
                    auto taskGuard = View->Queue->GetTask(taskId);
                    if (taskGuard->HasData()) {
                        DEBUG_LOG << "Add locked from unknown: " << taskId << Endl;
                        View->AddLockedTask(taskGuard, false);
                    } else if (taskGuard->IsLocked()) {
                        DEBUG_LOG << "remove incorrect locked: " << taskId << Endl;
                        View->RemoveTaskFromView(taskId);
                    } else {
                        DEBUG_LOG << "Add external locked: " << taskId << Endl;
                        View->AddExternalLocked(taskId);
                    }
                    View->TasksUnknownSignal.Signal(View->TasksUnknown.Size());
                } catch (...) {
                    ERROR_LOG << "Cannot take tasks from TasksUnknown queue: " << CurrentExceptionMessage() << Endl;
                }
            }
        }
    }
};

class TDistributedQueueView::TRelockTasksWatcher: public IObjectInQueue {
private:
    TDistributedQueueView* View;
public:

    TRelockTasksWatcher(TDistributedQueueView* view)
        : View(view) {

    }

    void Process(void* /*threadSpecificResource*/) override {
        while (View->IsActive()) {
            TString taskId;
            while (View->TasksRelock.Get(&taskId)) {
                try {
                    DEBUG_LOG << "Relock taken: " << taskId << Endl;
                    auto taskGuard = View->Queue->GetTask(taskId);
                    if (taskGuard->HasData()) {
                        DEBUG_LOG << "Add locked from relock: " << taskId << Endl;
                        View->AddLockedTask(taskGuard, true);
                    } else if (taskGuard->IsLocked()) {
                        DEBUG_LOG << "remove: " << taskId << Endl;
                        View->RemoveTaskFromView(taskId);
                    } else {
                        DEBUG_LOG << "External lock: " << taskId << Endl;
                        View->AddExternalLocked(taskId);
                    }
                    View->TasksRelockSignal.Signal(View->TasksRelock.Size());
                } catch (...) {
                    ERROR_LOG << "Cannot take tasks from TasksRelock queue: " << CurrentExceptionMessage() << Endl;
                }
            }
        }
    }
};

class TDistributedQueueView::TExternalLocksTasksWatcher: public IObjectInQueue {
private:
    TDistributedQueueView* View;
public:

    TExternalLocksTasksWatcher(TDistributedQueueView* view)
        : View(view) {

    }

    void Process(void* /*threadSpecificResource*/) override {
        while (View->IsActive()) {
            TString taskId;
            TVector<TString> notActual;
            while (View->TasksExternalLock.Get(&taskId)) {
                try {
                    DEBUG_LOG << "check external locked: " << taskId << Endl;
                    auto taskGuard = View->Queue->GetTask(taskId);
                    if (taskGuard->HasData()) {
                        DEBUG_LOG << "Add locked from external locked: " << taskId << Endl;
                        View->AddLockedTask(std::move(taskGuard), false);
                    } else if (taskGuard->IsLocked()) {
                        DEBUG_LOG << "remove external locked: " << taskId << Endl;
                        View->RemoveTaskFromView(taskId);
                    } else {
                        notActual.push_back(taskId);
                    }
                    View->TasksExternalLockSignal.Signal(View->TasksExternalLock.Size());
                } catch (...) {
                    ERROR_LOG << "Cannot take tasks from TasksExternalLock queue: " << CurrentExceptionMessage() << Endl;
                }
            }
            for (auto&& i : notActual) {
                View->TasksExternalLock.Put(i);
            }
            Sleep(TDuration::Seconds(5));
        }
    }
};

void TDistributedQueueView::AddExternalLocked(const TString& taskId) {
    TGuard<TMutex> g(Mutex);
    auto it = TasksInfo.find(taskId);
    CHECK_WITH_LOG(it != TasksInfo.end()) << taskId << " not found";
    TasksExternalLock.Put(taskId);
    TasksExternalLockSignal.Signal(TasksExternalLock.Size());
}

void TDistributedQueueView::AddRelock(const TString& taskId) {
    TGuard<TMutex> g(Mutex);
    DEBUG_LOG << "Add relock: " << taskId << Endl;
    auto it = TasksInfo.find(taskId);
    CHECK_WITH_LOG(it != TasksInfo.end());
    CHECK_WITH_LOG(!it->second->IsLocked()) << taskId << Endl;
    CHECK_WITH_LOG(it->second->HasData());
    TasksRelock.Put(taskId);
    TasksRelockSignal.Signal(TasksRelock.Size());
}

bool TDistributedQueueView::AddLockedTask(TTaskGuard::TPtr guard, const bool force) {
    DEBUG_LOG << "Add locked: " << guard->GetTaskIdentifier() << " locked size: " << TasksLocked.Size() << Endl;
    CHECK_WITH_LOG(guard->IsLocked());
    CHECK_WITH_LOG(guard->HasData());
    if (Selector && !Selector->IsAvailable(guard->GetTaskIdentifier())) {
        DEBUG_LOG << "Skip locked by selector: " << guard->GetTaskIdentifier() << Endl;
        return true;
    }

    TGuard<TMutex> g(Mutex);

    auto it = TasksInfo.find(guard->GetTaskIdentifier());
    bool newTask = false;
    if (it != TasksInfo.end()) {
        CHECK_WITH_LOG(!it->second->IsLocked());
        INFO_LOG << "exchange_task " << guard->GetTaskIdentifier() << Endl;
        it->second = guard;
    } else {
        DEBUG_LOG << "emplace locked: " << guard->GetTaskIdentifier() << Endl;
        TasksInfo.emplace(guard->GetTaskIdentifier(), guard);
        newTask = true;
    }
    guard->SetQueueActor(this);
    if (!force) {
        DEBUG_LOG << "!force locked: " << guard->GetTaskIdentifier() << Endl;
        if (guard->GetData().GetStartInstant() > Now()) {
            if (newTask) {
                Scheduler.Schedule(new TDeferredTask(this, guard));
                TasksScheduledSignal.Signal(Scheduler.Size());
            }
            DEBUG_LOG << "deferred locked: " << guard->GetTaskIdentifier() << Endl;
            return true;
        }
        if (guard->GetData().GetWaitTasks().size()) {
            g.Release();
            if (TasksWaiter->RegisterWaiter(guard)) {
                return true;
            }
        }
    }

    if (LockedTasksLimit && TasksLocked.Size() >= LockedTasksLimit) {
        auto it = TasksInfo.find(guard->GetTaskIdentifier());
        CHECK_WITH_LOG(it != TasksInfo.end());
        it->second->Unlock();
        TasksInfo.erase(it);
        DEBUG_LOG << "LockedTasksLimit reached: " << TasksLocked.Size() << ", release lock on " << guard->GetTaskIdentifier() << Endl;
        return false;
    }

    DEBUG_LOG << "add locked: " << guard->GetTaskIdentifier() << Endl;
    TasksLocked.Put(guard->GetTaskIdentifier());
    TasksLockedSignal.Signal(TasksLocked.Size());
    return true;
}

bool TDistributedQueueView::AddExistanceTask(const TString& taskId) {
    {
        TGuard<TMutex> gStart(TasksStartMutex);
        if (TasksStart.contains(taskId)) {
            return true;
        }
    }
    TGuard<TMutex> g(Mutex);
    if (LockedTasksLimit && TasksUnknown.Size() >= LockedTasksLimit) {
        return false;
    }
    auto it = TasksInfo.find(taskId);
    if (it == TasksInfo.end()) {
        DEBUG_LOG << "Add existance: " << taskId << Endl;
        TasksInfo.emplace(taskId, MakeAtomicShared<TTaskGuard>());
        TasksUnknown.Put(taskId);
        TasksUnknownSignal.Signal(TasksUnknown.Size());
    }
    return true;
}

void TDistributedQueueView::RemoveTaskFromView(const TString& taskId) {
    TGuard<TMutex> g(Mutex);
    auto it = TasksInfo.find(taskId);
    if (it != TasksInfo.end()) {
        if (it->second->IsLocked()) {
            if (it->second->HasData()) {
                const i64 counter = AtomicDecrement(ActiveTasksCounter);
                CHECK_WITH_LOG(counter >= 0);
                TasksActiveSignal.Signal(counter);
            }
            if (it->second->DataRestored()) {
                TasksActivityDurationSignal.Signal((Now() - it->second->GetLockDataInstant()).MilliSeconds());
            }
        }
        TasksInfo.erase(it);
    }
}

bool TDistributedQueueView::Process(IMessage* message) {
    TCollectServerInfo* messageInfo = dynamic_cast<TCollectServerInfo*>(message);
    if (messageInfo) {
        TGuard<TMutex> g(Mutex);
        NJson::TJsonValue info;
        info["watchers"] = Watchers.Size();
        info["scheduler"] = Scheduler.Size();
        info["tasks"] = TasksInfo.size();
        info["tasks_unknown"] = TasksUnknown.Size();
        info["tasks_relock"] = TasksRelock.Size();
        info["tasks_external_lock"] = TasksExternalLock.Size();
        info["tasks_locked"] = TasksLocked.Size();
        info["tasks_active"] = AtomicGet(ActiveTasksCounter);

        messageInfo->Fields["Executor_" + Name()] = info;
        return true;
    }
    return false;
}

void TDistributedQueueView::Start() {
    CHECK_WITH_LOG(AtomicCas(&IsActiveFlag, 1, 0));
    Watchers.Start(ThreadsCount * 2 + 2);
    Watchers.SafeAddAndOwn(MakeHolder<TQueueWatcher>(this));
    Watchers.SafeAddAndOwn(MakeHolder<TExternalLocksTasksWatcher>(this));
    for (ui32 i = 0; i < ThreadsCount; ++i) {
        Watchers.SafeAddAndOwn(MakeHolder<TUnknownTasksWatcher>(this));
    }
    for (ui32 i = 0; i < ThreadsCount; ++i) {
        Watchers.SafeAddAndOwn(MakeHolder<TRelockTasksWatcher>(this));
    }
    RegisterGlobalMessageProcessor(this);
}


TDistributedQueueView::TDistributedQueueView(IDistributedQueue::TPtr queue, const IDistributedTasksQueueConfig& config)
    : TasksActivityDurationSignal(queue->GetName())
    , TasksActivationDelaySignal(queue->GetName())
    , QueueSizeSignal(queue->GetName())
    , TasksUnknownSignal(queue->GetName())
    , TasksRelockSignal(queue->GetName())
    , TasksScheduledSignal(queue->GetName())
    , TasksLockedSignal(queue->GetName())
    , TasksExternalLockSignal(queue->GetName())
    , TasksActiveSignal(queue->GetName())
    , TasksCallbacksSignal(queue->GetName())
    , TasksOverflowSignal(queue->GetName())
    , Queue(queue)
    , LockedTasksLimit(config.GetLockedTasksLimit())
    , ReadQueueTimeout(config.GetReadQueueTimeout())
    , ReadQueueAttempts(config.GetReadQueueAttempts())
    , ReadQueueFailTimeout(config.GetReadQueueFailTimeout())
    , PingPeriod(config.GetPingPeriod())
    , TasksWaiter(MakeHolder<TTasksWaiter>(this))
{
}

bool TDistributedQueueView::Enqueue(const TTaskData& data, const bool rewrite /*= false*/) {
    TGuardTaskStart gts(data.GetTaskIdentifier(), this);
    {
        TGuard<TMutex> g(Mutex);
        if (!rewrite && TasksInfo.contains(data.GetTaskIdentifier())) {
            return false;
        }
    }
    TTaskGuard::TPtr result = Queue->PutTask(data, rewrite);
    {
        TWriteGuard wg(NodesMutex);
        Nodes.emplace(data.GetTaskIdentifier());
        NewNodes.emplace(data.GetTaskIdentifier());
    }
    if (!!result && result->IsLocked() && result->HasData()) {
        AddLockedTask(result, false);
        return true;
    }
    return false;
}

TTaskGuard::TPtr TDistributedQueueView::Dequeue() {
    TString taskId;
    if (TasksLocked.Get(&taskId)) {
        TGuard<TMutex> g(Mutex);
        if (LockedTasksLimit && AtomicGet(ActiveTasksCounter) >= static_cast<i64>(LockedTasksLimit)) {
            DEBUG_LOG << "release locked: " << taskId << ", active tasks: " << AtomicGet(ActiveTasksCounter) << ", limit " << LockedTasksLimit << Endl;
            auto it = TasksInfo.find(taskId);
            CHECK_WITH_LOG(it != TasksInfo.end());
            it->second->Unlock();
            TasksInfo.erase(it);
            return nullptr;
        }
        DEBUG_LOG << "take locked: " << taskId << ", active tasks: " << AtomicGet(ActiveTasksCounter) << ", limit " << LockedTasksLimit << Endl;
        TasksActiveSignal.Signal(AtomicIncrement(ActiveTasksCounter));
        TasksLockedSignal.Signal(TasksLocked.Size());
        auto it = TasksInfo.find(taskId);
        CHECK_WITH_LOG(it != TasksInfo.end());
        TasksActivationDelaySignal.Signal(it->second->GetDequeueDelay().MilliSeconds());
        return it->second;
    }
    return nullptr;
}

TNoTasksSelector::TFactory::TRegistrator<TNoTasksSelector> TNoTasksSelector::Registrator("no_tasks");
TAllTasksSelector::TFactory::TRegistrator<TAllTasksSelector> TAllTasksSelector::Registrator("all_tasks");
