#include "common.h"
#include <util/random/random.h>
#include <kernel/common_server/library/executor/abstract/data.h>
#include <kernel/common_server/util/logging/tskv_log.h>
#include <util/string/type.h>

bool TRestoreDataMeta::IsExistsCompatible(const IDistributedData* data) const {
    CHECK_WITH_LOG(Existance != EExistanceRequirement::LockPool);
    const bool result =
        (Existance == EExistanceRequirement::Optional) ||
        (Existance == EExistanceRequirement::Exists && !!data && !data->GetIsFinished()) ||
        (Existance == EExistanceRequirement::Finished && !!data && data->GetIsFinished()) ||
        (Existance == EExistanceRequirement::NoExists && (!data || data->GetIsFinished()));
    if (!result) {
        WARNING_LOG << Existance << " : " << (!!data ? ToString(data->GetIsFinished()) : "null") << Endl;
    }
    return result;
}

void TRestoreDataMeta::SerializeToProto(NFrontendProto::TRestoreDataMeta& proto) const {
    proto.SetId(Identifier);
    proto.SetExistanceRequirement((ui32)Existance);
    proto.SetIsReadOnly(IsReadOnly);
    proto.SetLockPoolSize(LockPoolSize);
}

bool TRestoreDataMeta::ParseFromProto(const NFrontendProto::TRestoreDataMeta& proto) {
    Identifier = proto.GetId();
    Existance = (EExistanceRequirement)proto.GetExistanceRequirement();
    IsReadOnly = proto.GetIsReadOnly();
    LockPoolSize = proto.GetLockPoolSize();
    return true;
}

bool TTaskGuard::LockData() noexcept {
    CHECK_WITH_LOG(!!TaskData);
    for (auto&& i : TaskData->GetRestoreData()) {
        try {
            NRTProc::TAbstractLock::TPtr lock;
            if (i.GetExistanceRequirement() == EExistanceRequirement::LockPool) {
                if (i.GetLockPoolSize()) {
                    ui32 idxStart = RandomNumber<ui32>() % i.GetLockPoolSize();
                    for (ui32 lockIdx = 1; lockIdx <= i.GetLockPoolSize(); ++lockIdx) {
                        const TString lockName = "data_lock--" + i.GetIdentifier() + "--" + ToString((idxStart + lockIdx) % i.GetLockPoolSize());
                        lock = Queue->WriteLock(lockName, TDuration::Zero());
                        if (!!lock) {
                            break;
                        }
                    }
                }
            } else {

                TMap<TString, TString> dataInfo;
                NUtil::TTSKVRecordParser::Parse<';', '='>(i.GetIdentifier(), dataInfo);

                const TString lockName = "data_lock--" + i.GetIdentifier();
                if (IsTrue(dataInfo["fake_lock"])) {
                    lock = Queue->FakeLock(lockName);
                } else if (i.GetIsReadOnly()) {
                    lock = Queue->ReadLock(lockName, TDuration::Zero());
                } else {
                    lock = Queue->WriteLock(lockName, TDuration::Zero());
                }
            }
            if (!lock) {
                DEBUG_LOG << "Cannot lock data '" << i.GetIdentifier() << "' for task " << GetTaskIdentifier() << Endl;
                DataLocks.clear();
                return false;
            } else {
                DataLocks.push_back(lock);
            }
        } catch (...) {
            ERROR_LOG << "lock taken failed for " << i.GetIdentifier() << ": " << CurrentExceptionMessage() << Endl;
            return false;
        }
    }
    LockDataInstant = Now();
    return true;
}

void TTaskGuard::UnlockData() noexcept {
    DataLocks.clear();
}

bool TTaskData::ParseFromProto(const NFrontendProto::TTaskData& taskData) {
    TaskIdentifier = taskData.GetTaskIdentifier();
    if (taskData.HasStartInstant()) {
        StartInstant = TInstant::Seconds(taskData.GetStartInstant());
    }
    EnqueueInstant = TInstant::Seconds(taskData.GetEnqueueInstant());
    if (taskData.HasDeadline()) {
        Deadline = TInstant::Seconds(taskData.GetDeadline());
    }
    for (auto&& i : taskData.GetData()) {
        TRestoreDataMeta meta;
        if (!meta.ParseFromProto(i))
            return false;
        AddData(meta);
    }
    for (auto&& i : taskData.GetWaitTask()) {
        WaitTasks.emplace(i);
    }
    return true;
}

void TTaskData::SerializeToProto(NFrontendProto::TTaskData& result) const {
    CHECK_WITH_LOG(!IsLocalFlag);
    result.SetTaskIdentifier(TaskIdentifier);
    if (StartInstant != TInstant::Zero()) {
        result.SetStartInstant(StartInstant.Seconds());
    }
    result.SetEnqueueInstant(EnqueueInstant.Seconds());
    if (Deadline != TInstant::Max()) {
        result.SetDeadline(Deadline.Seconds());
    }
    for (auto&& i : Data) {
        i.SerializeToProto(*result.AddData());
    }
    for (auto&& i : WaitTasks) {
        result.AddWaitTask(i);
    }
}
