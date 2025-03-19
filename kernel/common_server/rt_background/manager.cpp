#include "manager.h"
#include <kernel/common_server/library/unistat/signals.h>
#include <util/system/hostname.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/library/storage/abstract.h>
#include <kernel/common_server/library/logging/record/record.h>
#include <kernel/common_server/locks/abstract.h>
#include <util/generic/guid.h>

bool TExecutionAgent::FlushStatus(const TString& status) {
    ProcessState.SetProcessName(ProcessSettings.GetName()).SetStatus(status);
    ProcessState.SetHostName(GetFQDNHostName());
    ProcessState.SetLastFlush(Now());
    return FlushState(ProcessState);
}

bool TExecutionAgent::FlushActiveState(IRTBackgroundProcessState::TPtr state) {
    ProcessState.SetStatus("ACTIVE");
    ProcessState.SetHostName(GetFQDNHostName());
    ProcessState.SetProcessState(state);
    ProcessState.SetLastFlush(Now());
    return FlushState(ProcessState);
}

bool TExecutionAgent::FlushState(const TRTBackgroundProcessStateContainer& stateContainer) const {
    auto gLogging = TFLRecords::StartContext().Method("TExecutionAgent::FlushState");
    auto table = Database->GetTable("rt_background_state");
    const TInstant start = Now();
    while (Now() - start < Config.GetFinishAttemptionsTimeout()) {
        auto transaction = Database->CreateTransaction(false);
        NStorage::TTableRecord trUpdate = stateContainer.SerializeToTableRecord();

        NStorage::TTableRecord trCondition;
        trCondition.Set("bp_name", ProcessSettings.GetName());

        auto result = table->Upsert(trUpdate, transaction, trCondition);
        if (!!result && result->IsSucceed() && transaction->Commit()) {
            return true;
        } else {
            TFLEventLog::Log("request_problem");
        }
        Sleep(Config.GetFinishAttemptionsPause());
    }
    S_FAIL_LOG << "Cannot flush state for " << stateContainer.GetProcessName() << Endl;
    return false;
}

namespace {
    TMutex MutexUnlockableTasks;
    TMap<TString, TInstant> InstantStartUnlockableTasks;
}

bool TExecutionAgent::FinishState(IRTBackgroundProcessState::TPtr state) {
    ProcessState.SetStatus("SLEEPING");
    ProcessState.SetHostName(GetFQDNHostName());
    const TInstant now = Now();
    if (ProcessSettings->IsSimultaneousProcess()) {
        TGuard<TMutex> g(MutexUnlockableTasks);
        InstantStartUnlockableTasks[ProcessSettings.GetName()] = now;
    }

    ProcessState.SetLastExecution(now);
    ProcessState.SetLastFlush(now);
    if (!!state) {
        ProcessState.SetProcessState(state);
    }
    return FlushState(ProcessState);
}

bool TExecutionAgent::CheckState(const TRTBackgroundProcessStateContainer& processState) const {
    TInstant lastExecution;
    if (!ProcessSettings->GetTimeRestrictions().Empty() && !ProcessSettings->GetTimeRestrictions().IsActualNow(ModelingNow())) {
        TFLEventLog::Debug("not actual through time restrictions");
        return false;
    }
    if (ProcessSettings->IsSimultaneousProcess()) {
        TGuard<TMutex> g(MutexUnlockableTasks);
        auto it = InstantStartUnlockableTasks.find(ProcessSettings.GetName());
        lastExecution = (it == InstantStartUnlockableTasks.end()) ? TInstant::Zero() : it->second;
    } else {
        lastExecution = processState.GetLastExecution();
    }
    if (!!processState && ProcessSettings->GetNextStartInstant(lastExecution) > Now()) {
        TFLEventLog::Debug("Too young start")("last_execution", lastExecution)("now", Now());
        return false;
    }
    return true;
}

TString TExecutionAgent::GetLockName() const {
    return ProcessSettings->NeedGroupLock() ? "lock_process_group_" + ProcessSettings->GetType() : "lock_process_" + ProcessSettings.GetName();
}

bool TExecutionAgent::StartExecution(const TRTBackgroundManager& owner, TStates& states) {
    if (!ProcessSettings.GetEnabled()) {
        return false;
    }

    if (!CheckState(ProcessState)) {
        return false;
    }

    if (!ProcessSettings->IsSimultaneousProcess()) {
        ExternalLock = Server.GetLocksManager().Lock(GetLockName(), false, TDuration::Zero());
    } else {
        ExternalLock = MakeAtomicShared<NRTProc::TFakeLock>("fake");
    }
    InternalLock = NNamedLock::TryAcquireLock(GetLockName());
    if (!ExternalLock || !InternalLock) {
        TFLEventLog::Info("Cannot lock background process")("internal", !!InternalLock)("external", !!ExternalLock);
        return false;
    }
    if (!owner.RefreshState(states, ProcessSettings.GetName())) {
        return false;
    }
    ProcessState = TRTBackgroundManager::GetProcessState(states, ProcessSettings.GetName());
    if (!CheckState(ProcessState)) {
        return false;
    }
    ProcessState.SetStatus("ACTIVE");
    ProcessState.SetHostName(GetFQDNHostName());
    ProcessState.SetLastFlush(Now());
    return FlushState(ProcessState);
}

void TExecutionAgent::Process(void* /*threadSpecificResource*/) {
    auto gLogging = TFLRecords::StartContext().Method("TExecutionAgent::Process")
        .SignalId("rt_background_process")("&rt_process", ProcessSettings.GetName())("&rt_class_name", ProcessSettings.GetClassName());
    TFLEventLog::JustLSignal("", 1)("&code", "running");
    try {
        TTimeGuardImpl<false, TLOG_INFO> g("Process execution " + ProcessSettings.GetName());
        IRTBackgroundProcess::TExecutionContext context(Server, *this);
        context.SetLastFlush(ProcessState.GetLastFlush());
        IRTBackgroundProcessState::TPtr nextState = ProcessSettings->Execute(ProcessState.GetState(), context);
        if (!nextState) {
            TFLEventLog::Signal()("&code", "fail").Error("Cannot execute process");
        } else {
            TFLEventLog::JustSignal()("&code", "success");
        }
        FinishState(nextState);
    } catch (...) {
        TFLEventLog::Signal()("&code", "exception")("message", CurrentExceptionMessage());
    }
    TFLEventLog::JustLSignal("", 0)("&code", "running");
}

bool TRTBackgroundManager::WaitObjectInactive(const TString& processName, const TInstant deadline) const {
    TWriteGuard rg(Mutex);
    while (Now() < deadline) {
        TMap<TString, TRTBackgroundProcessStateContainer> states;
        if (GetStatesInfo(states)) {
            auto it = states.find(processName);
            if (it == states.end()) {
                return true;
            }
            if (it->second.GetStatus() == "SLEEPING") {
                return true;
            }
        }
        Sleep(TDuration::Seconds(1));
    }
    return false;
}

TRTBackgroundProcessStateContainer TRTBackgroundManager::GetProcessState(const TStates& states, const TString& processName) {
    TRTBackgroundProcessStateContainer stateContainer;
    auto it = states.find(processName);
    if (it == states.end()) {
        stateContainer = TRTBackgroundProcessStateContainer(MakeAtomicShared<IRTBackgroundProcessState>());
        stateContainer.SetLastExecution(TInstant::Zero());
        stateContainer.SetProcessName(processName);
    } else {
        stateContainer = it->second;
    }
    return stateContainer;
}

bool TRTBackgroundManager::Refresh() {
    TReadGuard rg(Mutex);
    TStates states;
    if (!GetStatesInfo(states)) {
        return false;
    }
    TMap<TString, TRTBackgroundProcessContainer> objects;
    if (!Storage->GetObjects(objects)) {
        TFLEventLog::Alert("cannot read objects for execute rt-background");
        return false;
    }
    for (auto&& [processId, process] : objects) {
        auto gLogging = TFLRecords::StartContext()("&rt_process", processId)("&rt_class_name", process.GetClassName());
        auto stateContainer = GetProcessState(states, processId);
        if (stateContainer.GetLastExecution()) {
            const TDuration d = process.GetEnabled() ? (Now() - stateContainer.GetLastExecution()) : TDuration::Zero();
            TFLEventLog::JustLSignal("rt_background_state", d.MilliSeconds())("&code", "freshness")("&status", stateContainer.GetStatus());

            const TDuration realTimeout = process->GetTimeoutMaybe().GetOrElse(Server.GetSettings().GetValueDef<TDuration>("rt_background.defaults.timeout", TDuration::Minutes(10)));
            const ui32 timeoutSignal = (stateContainer.GetStatus() == "ACTIVE" && d > realTimeout) ? 1 : 0;
            TFLEventLog::JustLSignal("rt_background_state", timeoutSignal)("&code", "timeout");
        }
        TFLEventLog::JustLSignal("rt_background_state", process.GetEnabled() ? 0 : 1)("&code", "disabled");
        if (!process->IsHostAvailable(Server)) {
            continue;
        }
        auto ea = MakeHolder<TExecutionAgent>(std::move(stateContainer), process, Server, Database, Config);
        if (!ea->StartExecution(*this, states)) {
            continue;
        }
        TasksQueue.SafeAddAndOwn(THolder(ea.Release()));
    }

    return true;
}

bool TRTBackgroundManager::RefreshState(TMap<TString, TRTBackgroundProcessStateContainer>& states, const TString& processName) const {
    auto table = Database->GetTable("rt_background_state");
    NCS::NStorage::TRecordsSetWT records;
    {
        auto transaction = Database->CreateTransaction(false);
        TSRSelect select("rt_background_state", &records);
        select.InitCondition<TSRBinary>("bp_name", processName);

        auto result = transaction->ExecRequest(select);
        if (!result->IsSucceed()) {
            TFLEventLog::Error("Cannot refresh data for rt_background_manager process");
            return false;
        }
    }
    for (auto&& i : records) {
        TRTBackgroundProcessStateContainer container;
        if (!container.DeserializeFromTableRecord(i)) {
            TFLEventLog::Error("Cannot parse info from record")("raw_data", i.SerializeToString());
            continue;
        }
        states[container.GetProcessName()] = std::move(container);
    }
    return true;
}

bool TRTBackgroundManager::GetStatesInfo(TStates& states) const {
    TRecordsSetWT records;
    {
        auto transaction = Database->CreateTransaction(false);
        TSRSelect select("rt_background_state", &records);
        auto result = transaction->ExecRequest(select);
        if (!result->IsSucceed()) {
            ERROR_LOG << "Cannot refresh data for rt_background_manager" << Endl;
            return false;
        }
    }
    TStates localStates;
    for (auto&& i : records) {
        TRTBackgroundProcessStateContainer container;
        if (!container.DeserializeFromTableRecord(i)) {
            WARNING_LOG << "Cannot parse info from record: " << i.SerializeToString() << Endl;
            continue;
        }
        localStates.emplace(container.GetProcessName(), std::move(container));
    }
    std::swap(localStates, states);
    return true;
}
