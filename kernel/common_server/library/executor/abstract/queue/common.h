#pragma once
#include <kernel/common_server/library/executor/proto/task.pb.h>
#include <util/generic/vector.h>
#include <util/datetime/base.h>
#include <kernel/common_server/library/storage/abstract.h>
#include "abstract.h"
#include <util/generic/object_counter.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/instant_model.h>

class IDistributedData;

enum class EExistanceRequirement {
    NoExists = 0,
    Optional = 1,
    Exists = 2,
    LockPool = 3,
    Finished = 4,
};

class TRestoreDataMeta {
private:
    TString Identifier;
    TString FindTag = "default";
    EExistanceRequirement Existance = EExistanceRequirement::Exists;
    ui32 LockPoolSize = 1;
    bool IsReadOnly = false;
public:
    explicit TRestoreDataMeta(const TString& id)
        : Identifier(id) {

    }

    ui32 GetLockPoolSize() const {
        return LockPoolSize;
    }

    bool IsExistsCompatible(const IDistributedData* data) const;

    TRestoreDataMeta& SetExistanceRequirement(const EExistanceRequirement value) {
        Existance = value;
        return *this;
    }

    TRestoreDataMeta& SetIsReadOnly(const bool value) {
        IsReadOnly = value;
        return *this;
    }

    TRestoreDataMeta& SetLockPoolSize(const ui32 value) {
        LockPoolSize = value;
        return *this;
    }

    TRestoreDataMeta& SetFindTag(const TString& tag) {
        FindTag = tag;
        return *this;
    }

    const TString& GetFindTag() const {
        return FindTag;
    }

    bool GetIsReadOnly() const {
        return IsReadOnly;
    }

    EExistanceRequirement GetExistanceRequirement() const {
        return Existance;
    }

    const TString& GetIdentifier() const {
        return Identifier;
    }

    TRestoreDataMeta() = default;

    void SerializeToProto(NFrontendProto::TRestoreDataMeta& proto) const;

    bool ParseFromProto(const NFrontendProto::TRestoreDataMeta& proto);
};

class TTaskData {
    RTLINE_ACCEPTOR(TTaskData, TaskIdentifier, TString, "");
    RTLINE_ACCEPTOR(TTaskData, Deadline, TInstant, TInstant::Max());
    RTLINE_ACCEPTOR(TTaskData, StartInstant, TInstant, TInstant::Zero());
    RTLINE_READONLY_ACCEPTOR(EnqueueInstant, TInstant, Now());
protected:
    TVector<TRestoreDataMeta> Data;
    TSet<TString> WaitTasks;
    bool IsLocalFlag = false;
public:
    using TPtr = TAtomicSharedPtr<TTaskData>;

    TTaskData() = default;
    TTaskData(const TTaskData& data) = default;
    TTaskData& operator= (const TTaskData& data) = default;

    explicit TTaskData(const TString& taskIdentifier, const TVector<TRestoreDataMeta>& data = TVector<TRestoreDataMeta>())
        : TaskIdentifier(taskIdentifier)
        , Data(data)
    {

    }

    explicit TTaskData(const TString& taskIdentifier, const TVector<TString>& waitTasks, const TVector<TRestoreDataMeta>& data)
        : TaskIdentifier(taskIdentifier)
        , Data(data)
        , WaitTasks(waitTasks.begin(), waitTasks.end())
    {

    }

    TTaskData& SetIsLocal(const bool value) {
        IsLocalFlag = value;
        return *this;
    }

    bool IsLocal() const {
        return IsLocalFlag;
    }

    bool IsCallback() const {
        return WaitTasks.size();
    }

    const TSet<TString>& GetWaitTasks() const {
        return WaitTasks;
    }

    TTaskData& AddWaitTask(const TString& task) {
        WaitTasks.emplace(task);
        return *this;
    }

    TTaskData& AddData(const TRestoreDataMeta& data) {
        Data.push_back(data);
        return *this;
    }

    bool ParseFromProto(const NFrontendProto::TTaskData& taskData);

    void SerializeToProto(NFrontendProto::TTaskData& result) const;

    bool HasStartInstant() const {
        return (StartInstant != TInstant::Zero());
    }

    bool HasDeadline() const {
        return Deadline != TInstant::Max();
    }

    bool IsExpired() const {
        return Deadline <= Now();
    }

    bool IsScheduledItem() const {
        return StartInstant != TInstant::Zero();
    }

    const TVector<TRestoreDataMeta>& GetRestoreData() const {
        return Data;
    }
};

class TTaskGuard: public TNonCopyable, public TObjectCounter<TTaskGuard> {
private:
    NRTProc::TAbstractLock::TPtr TaskLock = nullptr;
    TTaskData::TPtr TaskData = nullptr;
    IQueueActor* Queue = nullptr;
    TVector<NRTProc::TAbstractLock::TPtr> DataLocks;
    TInstant LockInstant = TInstant::Zero();
    TInstant LockDataInstant = TInstant::Zero();
public:

    TInstant GetLockInstant() const {
        CHECK_WITH_LOG(LockInstant != TInstant::Zero());
        return LockInstant;
    }

    bool DataRestored() const {
        return LockDataInstant != TInstant::Zero();
    }

    TInstant GetLockDataInstant() const {
        CHECK_WITH_LOG(LockDataInstant != TInstant::Zero());
        return LockDataInstant;
    }

    TDuration GetDequeueDelay() const {
        CHECK_WITH_LOG(!!TaskData);
        const TInstant now = Now();
        if (TaskData->HasStartInstant()) {
            if (now >= TaskData->GetStartInstant()) {
                return now - TaskData->GetStartInstant();
            } else {
                return TDuration::Max();
            }
        } else {
            if (now >= TaskData->GetEnqueueInstant()) {
                return now - TaskData->GetEnqueueInstant();
            } else {
                return TDuration::Max();
            }
        }
        return TDuration::Zero();
    }

    TTaskGuard* SetQueueActor(IQueueActor* queue) {
        Queue = queue;
        return this;
    }

    using TPtr = TAtomicSharedPtr<TTaskGuard>;

    TTaskGuard(IQueueActor* queue, NRTProc::TAbstractLock::TPtr lock, TTaskData::TPtr data)
        : TaskLock(lock)
        , TaskData(data)
        , Queue(queue) {
        LockInstant = Now();
    }

    TTaskGuard() {

    }

    void Unlock() {
        TaskLock = nullptr;
    }

    bool IsLocked() const {
        return !!TaskLock;
    }

    bool HasData() const {
        return !!TaskData;
    }

    const TString& GetTaskIdentifier() const {
        CHECK_WITH_LOG(!!TaskData);
        return TaskData->GetTaskIdentifier();
    }

    const TTaskData& GetData() const {
        CHECK_WITH_LOG(!!TaskData);
        return *TaskData;
    }

    bool LockData() noexcept;
    void UnlockData() noexcept;

    void RemoveTaskFromQueue() const {
        CHECK_WITH_LOG(!!TaskData);
        Queue->RemoveTask(TaskData->GetTaskIdentifier());
        INFO_LOG << "task " << TaskData->GetTaskIdentifier() << " removed" << Endl;
    }
};
