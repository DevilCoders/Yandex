#pragma once
#include <kernel/common_server/library/executor/abstract/queue.h>
#include <kernel/common_server/library/storage/abstract.h>
#include <kernel/common_server/util/lqueue.h>
#include <util/system/mutex.h>
#include <kernel/common_server/library/executor/proto/task.pb.h>
#include <kernel/common_server/library/unistat/signals.h>
#include <util/random/random.h>

class TVSQueueConfig: public IDistributedTasksQueueConfig {
private:
    static TFactory::TRegistrator<TVSQueueConfig> Registrator;
    NRTProc::TStorageOptions Options;
    IContextSignal<double>::TPtr SignalQueueSize;
    TString QueueIdentifier = "undefined_queue";
protected:

    virtual bool DoInit(const TYandexConfig::Section* section) override {
        AssertCorrectConfig(section->GetDirectives().GetValue("QueueIdentifier", QueueIdentifier), "Incorrect QueueIdentifier in configuration");
        auto child = section->GetAllChildren();
        auto it = child.find("Storage");
        AssertCorrectConfig(it != child.end(), "Incorrect configuration: no 'VStorage' section");
        AssertCorrectConfig(Options.Init(it->second), "Incorrect configuration: cannot initialize options for storage");
        SignalQueueSize = new TUnistatSignal<double>({"frontend-queue_size"}, EAggregationType::LastValue, "axxx");
        return true;
    }

    virtual void DoToString(IOutputStream& os) const override {
        os << "QueueIdentifier: " << QueueIdentifier << Endl;
        os << "<Storage>" << Endl;
        Options.ToString(os);
        os << "</Storage>" << Endl;
    }

public:

    const TString& GetQueueIdentifier() const {
        return QueueIdentifier;
    }

    const NRTProc::TStorageOptions& GetStorageOptions() const {
        return Options;
    }

    virtual TString GetName() const override {
        return "VStorage";
    }

    IDTasksQueue::TPtr Construct(IDistributedTaskContext* context) const override;
};

class TVSTasksQueue: public IDTasksQueue {
private:

    class TDistributedQueue: public IDistributedQueue {
    private:
        NRTProc::IVersionedStorage::TPtr Storage;
    protected:

        virtual NRTProc::TAbstractLock::TPtr WriteLock(const TString& key, const TDuration timeout) const override {
            return Storage->WriteLockNode(key, timeout);
        }

        virtual NRTProc::TAbstractLock::TPtr ReadLock(const TString& key, const TDuration timeout) const override {
            return Storage->ReadLockNode(key, timeout);
        }

        TTaskData::TPtr LoadTask(const TString& taskId) const override {
            NFrontendProto::TTaskData protoData;
            if (!Storage->GetValueProto("/tasks_queue/" + taskId, protoData, -1, false)) {
                DEBUG_LOG << "No data about task: " << "/tasks_queue/" << taskId << Endl;
                return nullptr;
            }
            auto result = MakeHolder<TTaskData>();
            if (!result->ParseFromProto(protoData)) {
                DEBUG_LOG << "Cannot parse " << protoData.DebugString() << Endl;
                return nullptr;
            }
            return result.Release();
        }

        bool SaveTask(const TTaskData& data, const bool rewrite) const override {
            NFrontendProto::TTaskData protoData;
            data.SerializeToProto(protoData);
            if (!rewrite && Storage->ExistsNode("/tasks_queue/" + data.GetTaskIdentifier())) {
                return false;
            }
            DEBUG_LOG << "Saving task " << data.GetTaskIdentifier() << " to queue" << Endl;
            Storage->SetValueProto("/tasks_queue/" + data.GetTaskIdentifier(), protoData, false, false);
            DEBUG_LOG << "Saved task " << data.GetTaskIdentifier() << " to queue" << Endl;
            return true;
        }

        virtual void RemoveTaskImpl(const TString& taskId) override {
            Storage->RemoveNode("/tasks_queue/" + taskId);
        }

        virtual TVector<TString> GetAllTasksImpl() const override {
            TVector<TString> nodes;
            Storage->GetNodes("/tasks_queue/", nodes, true);
            return nodes;
        }

        virtual ui32 GetSizeImpl() const override {
            TVector<TString> nodes;
            Storage->GetNodes("/tasks_queue/", nodes, true);
            return nodes.size();
        }

    public:

        using TPtr = TAtomicSharedPtr<IDistributedQueue>;

        TDistributedQueue(NRTProc::IVersionedStorage::TPtr storage, const TString& name)
            : IDistributedQueue(name)
            , Storage(storage)
        {
            Storage->SetValue("/tasks_queue", "", false, false);
        }

        virtual ~TDistributedQueue() = default;
    };

    const TVSQueueConfig Config;
    IContextSignal<double>::TPtr SignalQueueSize;
protected:

    virtual void DoStart() override {
    }

    virtual void DoStop() override {

    }
public:

    virtual ~TVSTasksQueue() = default;

    TVSTasksQueue(const TVSQueueConfig& config, IContextSignal<double>::TPtr signalQueueSize, IDistributedTaskContext* context)
        : IDTasksQueue(MakeAtomicShared<TDistributedQueue>(config.GetStorageOptions().ConstructStorage(), config.GetQueueIdentifier()), config, context)
        , Config(config)
        , SignalQueueSize(signalQueueSize)
    {
    }
};

