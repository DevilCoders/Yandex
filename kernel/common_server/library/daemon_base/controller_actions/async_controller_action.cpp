#include "async_controller_action.h"
#include <library/cpp/json/json_reader.h>

namespace NDaemonController {

    bool TControllerAsyncAction::ParseJson(const TString& str, NJson::TJsonValue& result) {
        if (!str) {
            Fail("Empty reply from server on restart command");
            return false;
        }
        TStringInput si(str);
        if (!NJson::ReadJsonTree(&si, &result)) {
            Fail("Reply is not json: " + str);
            return false;
        }
        return true;
    }

    TString TControllerAsyncAction::DoBuildCommandWait() const {
        if (!IdTask)
            Fail("There is no idTask");
        return "command=get_async_command_info&id=" + IdTask;
    }

    void TControllerAsyncAction::DoInterpretResultStart(const TString& result) {
        NJson::TJsonValue json;
        if (!ParseJson(result, json))
            return;
        if (!json["id"].GetString(&IdTask) || !IdTask) {
            Fail("there is no id: " + result);
            return;
        }
        if (AsyncPolicy() == apStart)
            Success("OK");
    }

    void TControllerAsyncAction::InterpretServerFailed() {
        Fail("server failed");
    }

    void TControllerAsyncAction::InterpretTaskReply(TAsyncTaskExecutor::TTask::TStatus taskStatus, const NJson::TJsonValue& result) {
        switch (taskStatus) {
        case TAsyncTaskExecutor::TTask::stsFinished:
            Success(result.GetStringRobust());
            break;
        case TAsyncTaskExecutor::TTask::stsFailed:
            Fail(result.GetStringRobust());
            break;
        case TAsyncTaskExecutor::TTask::stsNotFound:
            FAIL_LOG("Invalid behavior");
        default:
            break;
        }
    }

    void TControllerAsyncAction::DoInterpretResultWait(const TString& result) {
        NJson::TJsonValue json;
        if (!ParseJson(result, json))
            return;
        const NJson::TJsonValue& resultReply = json["result"];
        TAsyncTaskExecutor::TTask::TStatus status = FromString<TAsyncTaskExecutor::TTask::TStatus>(resultReply["task_status"].GetStringRobust());
        if (status == TAsyncTaskExecutor::TTask::stsNotFound)
            InterpretServerFailed();
        else
            InterpretTaskReply(status, resultReply);
    }
}
