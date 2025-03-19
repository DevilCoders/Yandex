#pragma once
#include "data.h"
#include "queue.h"
#include "context.h"
#include <util/thread/pool.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/generic/cast.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/json/writer/json_value.h>
#include "data_storage.h"

class IDistributedTask;
class IDistributedData;
class TTaskGuard;

class ITaskExecutor {
public:
    virtual ~ITaskExecutor() = default;
    virtual void ClearOld() const = 0;
    virtual void StoreLocalTask(TAtomicSharedPtr<IDistributedTask> task) const = 0;
    virtual bool StoreTask2(const IDistributedTask* task) const = 0;
    virtual void RemoveTask(const TTaskGuard& guard) const = 0;

    virtual bool StoreData2(const IDistributedData* data) const = 0;
    virtual void RemoveData(const TString& identifier) const = 0;

    virtual bool EnqueueTask(const TString& taskId, const TVector<IDTasksQueue::TRestoreDataMeta>& data, const TVector<TString>& waitTasks, const TInstant startInstant = TInstant::Zero(), const TInstant deadline = TInstant::Max()) const = 0;

    bool EnqueueTask(const TString& taskId, const IDTasksQueue::TRestoreDataMeta& data, const TString& waitTaskCallback = "", const TInstant startInstant = TInstant::Zero(), const TInstant deadline = TInstant::Max()) const {
        TVector<IDTasksQueue::TRestoreDataMeta> vData;
        if (!!data.GetIdentifier()) {
            vData.push_back(data);
        }
        TVector<TString> waitTasks;

        if (!!waitTaskCallback) {
            waitTasks.push_back(waitTaskCallback);
        }
        return EnqueueTask(taskId, vData, waitTasks, startInstant, deadline);
    }

    bool EnqueueTask(const TString& taskId, const TString& dataId, const TString& waitTaskCallback = "", const TInstant startInstant = TInstant::Zero(), const TInstant deadline = TInstant::Max()) const {
        IDTasksQueue::TRestoreDataMeta data(dataId);
        return EnqueueTask(taskId, data, waitTaskCallback, startInstant, deadline);
    }

    virtual bool IsActive() const = 0;

    virtual void RescheduleTask(const IDistributedTask* task, const TInstant scheduleInstant, bool unlockData = false) = 0;
    virtual TThreadPool& GetExecutor() = 0;
    virtual IDistributedTaskContext* GetContext() const = 0;
    template <class T>
    T& GetContextAs() const {
        CHECK_WITH_LOG(GetContext());
        return *VerifyDynamicCast<T*>(GetContext());
    }
};

class IDistributedTask: public TObjectCounter<IDistributedTask> {
public:
    using TFactory = NObjectFactory::TParametrizedObjectFactory<IDistributedTask, TString, TTaskConstructionContext>;
    using TPtr = TAtomicSharedPtr<IDistributedTask>;

    class TRemoveGuard {
    public:
        TRemoveGuard(TPtr task)
            : Task(task)
        {
        }
        ~TRemoveGuard() noexcept(false) {
            if (Task) {
                Task->Remove();
            }
        }

        void Release() {
            Task.Drop();
        }

    private:
        TPtr Task;
    };

private:
    class TEvent {
    private:
        TString Info;
        const TInstant Instant = Now();
    public:
        TEvent(const TString& info)
            : Info(info) {

        }

        TEvent(const NFrontendProto::TEventLogItem& info)
            : Info(info.GetEvent())
            , Instant(TInstant::MicroSeconds(info.GetInstant())) {

        }

        NFrontendProto::TEventLogItem SerializeToProto() const {
            NFrontendProto::TEventLogItem result;
            result.SetEvent(Info);
            result.SetInstant(Instant.MicroSeconds());
            return result;
        }

        NJson::TJsonValue SerializeToJson() const {
            NJson::TJsonValue result;
            result["instant"] = Instant.MicroSeconds();
            result["event"] = Info;
            return result;
        }
    };

    TMap<TString, IDDataStorage::TGuard::TPtr> Data;
protected:
    bool WriteEventLog = false;
    mutable TVector<TEvent> Events;

    bool HasEventLog() const {
        return Events.size();
    }

    NFrontendProto::TEventLog SerializeEventLogToProto() const {
        NFrontendProto::TEventLog result;
        for (auto&& i : Events) {
            *result.AddItem() = i.SerializeToProto();
        }
        return result;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeEventLogFromProto(const NFrontendProto::TEventLog& proto) {
        Events.clear();
        for (auto&& i : proto.GetItem()) {
            Events.emplace_back(i);
        }
        return true;
    }

    virtual NJson::TJsonValue DoGetInfo(const TCgiParameters* /*cgi*/) const {
        return NJson::TJsonValue(NJson::JSON_NULL);
    }
protected:
    IDTasksQueue::TGuard::TPtr QueueGuard;
    ITaskExecutor* Executor = nullptr;

    const TString Identifier;
    const TString Type;
    virtual bool DoExecute(IDistributedTask::TPtr self) noexcept = 0;
    virtual bool DoOnTimeout(IDistributedTask::TPtr /*self*/) noexcept {
        return true;
    }
    virtual bool DoOnIncorrectData(IDistributedTask::TPtr /*self*/) noexcept {
        return true;
    }
public:

    virtual void AddEvent(const TString& info) const final {
        Events.emplace_back(info);
    }

    virtual NJson::TJsonValue GetTaskInfo(const TCgiParameters* cgi = nullptr) const final;

    IDDataStorage::TGuard::TPtr GetData(const TString& tag = "default") const {
        auto it = Data.find(tag);
        if (it == Data.end()) {
            return nullptr;
        } else {
            return it->second;
        }
    }

    template <class T>
    T& GetData(const TString& tag = "default") const {
        auto it = Data.find(tag);
        CHECK_WITH_LOG(it != Data.end());
        return it->second->GetDataAs<T>();
    }

    void Remove() const {
        Executor->RemoveTask(*QueueGuard);
    }

    IDTasksQueue::TGuard::TPtr GetTaskGuard() const {
        return QueueGuard;
    }

    virtual ~IDistributedTask() = default;

    const TString& GetIdentifier() const {
        return Identifier;
    }

    IDistributedTask(const TTaskConstructionContext& context)
        : Identifier(context.GetIdentifier())
        , Type(context.GetType())
    {

    }

    IDistributedTask& SetWriteEventLog(const bool value) {
        WriteEventLog = value;
        return *this;
    }

    bool GetWriteEventLog() const {
        return WriteEventLog;
    }

    const TString& GetType() const {
        return Type;
    }

    void SerializeMetaToProto(NFrontendProto::TTaskMeta& proto) const;

    bool ParseMetaFromProto(const NFrontendProto::TTaskMeta& proto);

    virtual void Execute(IDTasksQueue::TGuard::TPtr queueGuard, TMap<TString, IDDataStorage::TGuard::TPtr> data, ITaskExecutor* executor, IDistributedTask::TPtr self) noexcept final;
    virtual void OnTimeout(IDTasksQueue::TGuard::TPtr queueGuard, TMap<TString, IDDataStorage::TGuard::TPtr> data, ITaskExecutor* executor, IDistributedTask::TPtr self) noexcept final;
    virtual void OnIncorrectData(IDTasksQueue::TGuard::TPtr queueGuard, TMap<TString, IDDataStorage::TGuard::TPtr> data, ITaskExecutor* executor, IDistributedTask::TPtr self) noexcept final;
    Y_WARN_UNUSED_RESULT virtual bool Deserialize(const TBlob& /*data*/) {
        return true;
    }

    virtual TBlob Serialize() const {
        return TBlob();
    }
};

