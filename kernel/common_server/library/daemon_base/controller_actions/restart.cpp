#include "restart.h"
#include "get_status.h"
#include <library/cpp/logger/global/global.h>
#include <kernel/daemon/common/common.h>
#include <kernel/daemon/async/async_task_executor.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/json/json_reader.h>
#include <util/stream/str.h>

namespace NDaemonController {

    NJson::TJsonValue TRestartAction::DoSerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("hard_stop", HardStop);
        result.InsertValue("abort_is_fail", AbortIsFail);
        result.InsertValue("stage", (int)Stage);
        result.InsertValue("stop_timeout", StopTimeout.ToString());
        return result;
    }

    void TRestartAction::DoDeserializeFromJson(const NJson::TJsonValue& json) {
        NJson::TJsonValue::TMapType map;
        CHECK_WITH_LOG(json.GetMap(&map));
        HardStop = map["hard_stop"].GetBooleanRobust();
        AbortIsFail = map["abort_is_fail"].GetBooleanRobust();
        Stage = (TStage)map["stage"].GetInteger();
        StopTimeout = TDuration::Parse(map["stop_timeout"].GetStringRobust());
    }

    TString TRestartAction::DoBuildCommandStart() const {
        return TString("async=yes&command=") + (HardStop ? TString("abort") : ("restart&reread_config=yes&rigid_level=1&stop_timeout=" + StopTimeout.ToString()));
    }

    TString TRestartAction::DoBuildCommandWait() const {
        switch (Stage) {
        case stWaitSoftRestart:
            return TControllerAsyncAction::DoBuildCommandWait();
        case stWaitHardRestart:
            return "command=get_status";
        default:
            FAIL_LOG("Invalid stage");
        }
    }

    void TRestartAction::SetStage(TStage stage) const {
        if (Stage == stage)
            return;
        DEBUG_LOG << "restart task: set stage " << (int)stage;
        Stage = stage;
    }

    void TRestartAction::InterpretServerFailed() {
        if (AbortIsFail)
            Fail("abort used");
        SetStage(stWaitHardRestart);
    }

    void TRestartAction::DoInterpretResultWait(const TString& result) {
        NJson::TJsonValue json;
        switch (Stage) {
        case stWaitSoftRestart:
            TControllerAsyncAction::DoInterpretResultWait(result);
            break;
        case stWaitHardRestart: {
            TStatusAction sa;
            TString pd;
            sa.BuildCommand(pd, "");
            sa.DoInterpretResult(result);
            if (sa.IsFailed())
                Fail(sa.GetInfo());
            else if (sa.GetInfo() == ToString(NController::ssbcActive))
                Success("OK");
            else if (sa.GetInfo() == ToString(NController::ssbcNotRunnable))
                Fail(ToString(NController::ssbcNotRunnable));
            break;
        }
        default:
            FAIL_LOG("Invalid stage");
        }
    }

    bool TRestartAction::GetNotContinuableTaskOnStarting() const {
        return false;
    }

    ui32 TRestartAction::GetAttemptionsMaxResend() const {
        return 2;
    }

    TRestartAction::TFactory::TRegistrator<TRestartAction> TRestartAction::Registrator(RESTART_ACTION_NAME);

}
