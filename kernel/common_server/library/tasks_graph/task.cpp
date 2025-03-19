#include "task.h"
#include <library/cpp/logger/global/global.h>
#include <util/stream/str.h>

namespace NRTYScript {

    TString TTaskContainer::GetStatusInfo() const {
        TString result;
        if (!Status)
            result += "E";
        if (HasStatus(StatusRunning))
            result += "R";
        if (HasStatus(StatusFailed))
            result += "F";
        if (HasStatus(StatusSuccess))
            result += "S";
        CHECK_WITH_LOG(HasStatus(StatusFailed) ^ HasStatus(StatusSuccess));
        return result;
    }

    void TTaskContainer::Execute(void* externalInfo) const {
        //if (Status & (StatusFailed | StatusSuccess))
        //    return;
        CHECK_WITH_LOG(!!Task);
        Status |= StatusRunning;
        try {
            if (Task->Execute(externalInfo)) {
                Status |= StatusSuccess;
                Status &= ~StatusFailed;
            } else {
                Status |= StatusFailed;
                Status &= ~StatusSuccess;
            }
        } catch (...) {
            TStringStream ss;
            ss << "Exception on task execution for " << GetName() << ": " << CurrentExceptionMessage();
            Task->Fail(ss.Str());
            ERROR_LOG << ss.Str() << Endl;
            Status |= StatusFailed;
            Status &= ~StatusSuccess;
        }
        Status &= ~StatusRunning;
    }

    void TTaskContainer::Deserialize(const NJson::TJsonValue& info) {
        Task = ITask::TFactory::Construct(info["class"].GetStringRobust());
        CHECK_WITH_LOG(!!Task);
        Status = info["status"].GetUIntegerRobust();
        Task->Deserialize(info["task"]);
        Name = info["name"].GetStringRobust();
    }

    TString TTaskContainer::GetName() const {
        return Name;
    }

    bool ITasksInfo::GetValueByPath(const TString& taskName, const TString& path, NJson::TJsonValue& result) const {
        NJson::TJsonValue taskInfo = GetTaskInfo(taskName);
        const NJson::TJsonValue* infoTask = taskInfo.GetValueByPath(path, '.');
        if (infoTask)
            result = *infoTask;
        return infoTask;
    }

    bool ITasksInfo::GetValueByPath(const TString& fullPath, NJson::TJsonValue& result) const {
        ui32 pos = fullPath.find_first_of('-');
        CHECK_WITH_LOG(pos < fullPath.size());
        NJson::TJsonValue taskInfo = GetTaskInfo(fullPath.substr(0, pos));
        const NJson::TJsonValue* infoTask = taskInfo.GetValueByPath(fullPath.substr(pos + 1), '.');
        if (infoTask)
            result = *infoTask;
        return infoTask;
    }

}
