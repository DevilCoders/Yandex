#pragma once
#include <kernel/common_server/library/executor/abstract/queue/view.h>
#include <kernel/common_server/library/executor/abstract/queue/abstract.h>
#include <kernel/common_server/library/executor/abstract/queue/common.h>

class IDistributedTasksQueueConfig;
class IDTasksStorage;

class IDTasksQueue {
private:
    TAtomic Activity = 0;
    THolder<TDistributedQueueView> QueueView;
    IDistributedQueue::TPtr OriginalQueue;
protected:
    virtual void DoStart() = 0;
    virtual void DoStop() = 0;
public:
    using TPtr = TAtomicSharedPtr<IDTasksQueue>;
    using TRestoreDataMeta = ::TRestoreDataMeta;
    using TTaskData = ::TTaskData;
    using TGuard = ::TTaskGuard;

    IDTasksQueue(IDistributedQueue::TPtr queue, const IDistributedTasksQueueConfig& config, IDistributedTaskContext* context);

    virtual ~IDTasksQueue() = default;

    IDistributedQueue::TPtr GetOriginalQueue() const {
        return OriginalQueue;
    }

    bool IsActive() const {
        return AtomicGet(Activity) == 1;
    }

    virtual void Start(const ui32 threadsCountView) final {
        CHECK_WITH_LOG(!IsActive());
        DoStart();
        QueueView->SetThreadsCount(threadsCountView);
        QueueView->Start();
        AtomicSet(Activity, 1);
    }

    virtual void Stop() final {
        CHECK_WITH_LOG(IsActive());
        AtomicSet(Activity, 0);
        QueueView->Stop();
        DoStop();
    }

    virtual bool Enqueue(const TTaskData& taskData, const bool rewrite) final {
        return QueueView->Enqueue(taskData, rewrite);
    }

    virtual TTaskGuard::TPtr Dequeue() final {
        return QueueView->Dequeue();
    }

    virtual ui32 GetSize(const bool noCache = true) const {
        return QueueView->GetSize(noCache);
    }

    virtual TVector<TString> GetNodes(const bool noCache = true) const {
        return QueueView->GetNodes(noCache);
    }

    virtual TVector<TString> GetAllNodes() const {
        return QueueView->GetAllNodes();
    }

    void ClearOld(IDTasksStorage& storage, const TAtomic& active, const TDuration borderWaiting) const;

    void SetSelector(IQueueViewSelector::TPtr selector) {
        QueueView->SetSelector(selector);
    }
};

class IDistributedTaskContext;

class IDistributedTasksQueueConfig {
    RTLINE_READONLY_ACCEPTOR(TasksSelectorName, TString, "all_tasks");
    RTLINE_READONLY_ACCEPTOR(LockedTasksLimit, ui64, 0);
    RTLINE_READONLY_ACCEPTOR(ReadQueueAttempts, ui64, 16);
    RTLINE_READONLY_ACCEPTOR(ReadQueueTimeout, TDuration, TDuration::Seconds(10));
    RTLINE_READONLY_ACCEPTOR(ReadQueueFailTimeout, TDuration, TDuration::Max());
    RTLINE_READONLY_ACCEPTOR(PingPeriod, TDuration, TDuration::Seconds(1));

protected:
    virtual bool DoInit(const TYandexConfig::Section* section) = 0;

    virtual bool Init(const TYandexConfig::Section* section) final {
        TasksSelectorName = section->GetDirectives().Value("TasksSelectorName", TasksSelectorName);
        AssertCorrectConfig(IQueueViewSelector::TFactory::Has(TasksSelectorName), "Incorrect tasks selector for executor");
        LockedTasksLimit = section->GetDirectives().Value("LockedTasksLimit", LockedTasksLimit);

        ReadQueueAttempts = section->GetDirectives().Value("ReadQueueAttempts", ReadQueueAttempts);
        ReadQueueTimeout = section->GetDirectives().Value("ReadQueueTimeout", ReadQueueTimeout);
        ReadQueueFailTimeout = section->GetDirectives().Value("ReadQueueFailTimeout", ReadQueueFailTimeout);
        PingPeriod = section->GetDirectives().Value("PingPeriod", PingPeriod);

        return DoInit(section);
    }
    virtual void DoToString(IOutputStream& os) const = 0;
public:
    using TFactory = NObjectFactory::TParametrizedObjectFactory<IDistributedTasksQueueConfig, TString>;
    using TPtr = TAtomicSharedPtr<IDistributedTasksQueueConfig>;

    virtual ~IDistributedTasksQueueConfig() = default;

    virtual TString GetName() const = 0;

    virtual void ToString(IOutputStream& os) const {
        os << "Type: " << GetName() << Endl;
        os << "TasksSelectorName: " << TasksSelectorName << Endl;
        os << "LockedTasksLimit: " << LockedTasksLimit << Endl;
        os << "ReadQueueAttempts: " << ReadQueueAttempts << Endl;
        os << "ReadQueueTimeout: " << ReadQueueTimeout << Endl;
        os << "ReadQueueFailTimeout: " << ReadQueueFailTimeout << Endl;
        os << "PingPeriod: " << PingPeriod << Endl;
        DoToString(os);
    }

    virtual IDTasksQueue::TPtr Construct(IDistributedTaskContext* context) const = 0;

    static IDistributedTasksQueueConfig::TPtr ConstructConfig(const TYandexConfig::Section* section) {
        TString type;
        AssertCorrectConfig(section->GetDirectives().GetValue("Type", type), "Can't read type for distributed queue in configuration");

        IDistributedTasksQueueConfig::TPtr result = TFactory::Construct(type);
        AssertCorrectConfig(!!result, "Can't construct type %s", type.data());
        AssertCorrectConfig(result->Init(section), "Can't initialize queue from configuration for type: '%s'", type.data());
        return result;
    }
};

