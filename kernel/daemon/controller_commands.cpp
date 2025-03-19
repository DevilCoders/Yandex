#include "bomb.h"
#include "base_controller.h"
#include "messages.h"

#include <kernel/daemon/common/common.h>
#include <kernel/daemon/common/json.h>

#include <library/cpp/threading/named_lock/named_lock.h>
#include <library/cpp/mediator/messenger.h>
#include <library/cpp/balloc/optional/operators.h>

using NController::TController;

#define DEFINE_COMMON_CONTROLLER_COMMAND(name) \
class T ## name ## Command : public TController::TCommandProcessor {\
protected:\
    virtual TString GetName() const { return #name ; }\
    virtual void DoProcess();\
};\
    TController::TCommandProcessor::TFactory::TRegistrator<T ## name ## Command> Registrator ## name(#name);\
    void T ## name ## Command::DoProcess() {

DEFINE_COMMON_CONTROLLER_COMMAND(restart)
    auto lock = NNamedLock::TryAcquireLock("controller_command:" + GetName());
    if (!lock) {
        Write("result", "already restarting");
        Write("status", ToString(Owner->GetStatus()));
        return;
    }

    const TCgiParameters& params = GetCgi();
    ui32 rigidStopLevel = ParseRigidStopLevel(params);
    bool reread = (params.Find("reread_config") != params.end()) && FromString<bool>(params.Get("reread_config"));
    const TString sleepStr = params.Get("sleep");
    TDuration sleepTimeout;
    if (!!sleepStr)
        sleepTimeout = TDuration::Parse(sleepStr);
    const TString stopTimeoutStr = params.Get("stop_timeout");
    TDuration stopTimeout;
    if (!!stopTimeoutStr)
        stopTimeout = TDuration::Parse(stopTimeoutStr);
    NController::TRestartServerStatistics statistics;
    TBomb bomb(stopTimeout, "stop on restart too long");
    Owner->DestroyServer(NController::TDestroyServerContext(rigidStopLevel).SetStatistics(&statistics).SetSleepTimeout(sleepTimeout));
    Owner->ClearServerDataStatus();
    bomb.Deactivate();
    SetStatus(stsStarted, "STOPPED");
    if (Owner->RestartServer(NController::TRestartContext(rigidStopLevel).SetForceConfigsReading(true).SetReread(reread).SetStatistics(&statistics).SetSleepTimeout(sleepTimeout))) {
        if (Owner->GetStatus() != NController::ssbcActive) {
            ythrow yexception() << Owner->GetStatus();
        }
    }
    Write("stop_time", statistics.StopTime.MilliSeconds());
    Write("start_time", statistics.StartTime.MilliSeconds());
    Write("rigid_level", rigidStopLevel);
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(reopenlog)
    TLogBackend::ReopenAllBackends();
    Owner->GetDaemonConfig()->ReopenLog();
    SendGlobalMessage<TMessageReopenLogs>();
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(clear_configurations)
    Owner->ClearConfigurations();
    Write("result", "OK");
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(clear_data_status)
    Owner->ClearServerDataStatus();
    Write("result", "OK");
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(get_status)
    Write("status", ToString(Owner->GetStatus()));
    const TServerInfo info = Owner->GetServerInfo(true);
    Write("result", info);
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(stop)
    const TCgiParameters& params = GetCgi();
    ui32 rigidStopLevel = ParseRigidStopLevel(params);
    const TString sleepStr = params.Get("sleep");
    TDuration sleepTimeout;
    if (!!sleepStr)
        sleepTimeout = TDuration::Parse(sleepStr);
    Owner->DestroyServer(NController::TDestroyServerContext(rigidStopLevel).SetSleepTimeout(sleepTimeout));
    Write("rigid_level", rigidStopLevel);
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(abort)
    if (IsTrue(GetCgi().Get("coredump"))) {
        FAIL_LOG("Aborted by user command");
    } else {
        NOTICE_LOG << "Soft abort invoked by user command" << Endl;
        _exit(EXIT_FAILURE);
    }
END_CONTROLLER_COMMAND

namespace {
    class TStopThread : public IThreadFactory::IThreadAble {
    private:
        TController* Owner;

    public:
        TStopThread(TController* owner)
            : Owner(owner)
        {
        }

        void DoExecute() override {
            ThreadDisableBalloc();
            THolder<TStopThread> suicide(this);
            Owner->Stop(0);
        }
    };
};

DEFINE_COMMON_CONTROLLER_COMMAND(shutdown)
    const TCgiParameters& params = GetCgi();
    ui32 rigidStopLevel = ParseRigidStopLevel(params, 1);
    const TString sleepStr = params.Get("sleep");
    TDuration sleepTimeout;
    if (!!sleepStr)
        sleepTimeout = TDuration::Parse(sleepStr);
    Owner->DestroyServer(NController::TDestroyServerContext(rigidStopLevel).SetSleepTimeout(sleepTimeout));
    SystemThreadFactory()->Run(new TStopThread(Owner));
END_CONTROLLER_COMMAND

DEFINE_COMMON_CONTROLLER_COMMAND(get_info_server)
    const TServerInfo info = Owner->GetServerInfo(false);
    if (GetCgi().Has("filter")) {
        NUtil::TJsonFilter filter;
        filter.SetIgnoreUnknownPath(true);
        filter.AddFilters(GetCgi(), "result.");
        NJson::TJsonValue filtered;
        filter.Apply(info, filtered);
        Write("result", filtered);
    } else
        Write("result", info);
END_CONTROLLER_COMMAND

#undef DEFINE_COMMON_CONTROLLER_COMMAND
