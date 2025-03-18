#include "master_target_graph.h"

#include "cron_state.h"
#include "http_static.h"
#include "master_config.h"
#include "master_profiler.h"
#include "messages.h"
#include "target_stats.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/constants.h>
#include <tools/clustermaster/common/log.h>

#include <kernel/yt/dynamic/client.h>

#include <library/cpp/digest/old_crc/crc.h>
#include <library/cpp/xsltransform/xsltransform.h>

#include <util/datetime/base.h>
#include <util/datetime/cputimer.h>
#include <util/random/random.h>
#include <util/string/split.h>
#include <util/string/util.h>
#include <util/system/hostname.h>
#include <util/system/sigset.h>
#include <util/system/tempfile.h>

#ifndef _win_
#   include <sys/wait.h>
#endif // !_win_


TMasterGraph::TMasterGraph(TMasterListManager* l, bool forTest)
    : TTargetGraphBase<TMasterGraphTypes>(l, forTest)
    , GraphvizCrc(0)
    , HasObservers(false)
    , PerformedPokeUpdates(0)
    , Variables(new TVariablesMap)
    , TvmClient(nullptr)
    , TelegramSecret()
    , JNSSecret()
{
    TFsPath tvmSecretFile(MasterOptions.TvmSecretPath);
    if (tvmSecretFile.IsFile()) {
        TString tvmSecret = Strip(TFileInput(tvmSecretFile).ReadAll());
        LOG("Reading tvm secret from " << MasterOptions.TvmSecretPath);
        NTvmAuth::NTvmApi::TClientSettings tvmSettings;
        tvmSettings.SetSelfTvmId(CLUSTERMASTER_TVM_CLIENT_ID);
        tvmSettings.EnableServiceTicketsFetchOptions(tvmSecret, {{"YaSms", YA_SMS_TVM_CLIENT_ID}, {"YaSmsTesting", YA_SMS_TESTING_TVM_CLIENT_ID}, {"Staff", STAFF_TVM_CLIENT_ID}});
        TvmClient = std::make_shared<NTvmAuth::TTvmClient>(tvmSettings, NTvmAuth::TDevNullLogger::IAmBrave());
    }
    TFsPath telegramSecretFile(MasterOptions.TelegramSecretPath);
    if (telegramSecretFile.IsFile()) {
        TelegramSecret = Strip(TFileInput(telegramSecretFile).ReadAll());
        LOG("Reading telegram secret from " << MasterOptions.TelegramSecretPath);
    }
    TFsPath jnsSecretFile(MasterOptions.JNSSecretPath);
    if (jnsSecretFile.IsFile()) {
        JNSSecret = Strip(TFileInput(jnsSecretFile).ReadAll());
        LOG("Reading jns secret from " << MasterOptions.JNSSecretPath);
    }
}

void TMasterGraph::ProcessFullStatus(TFullStatusMessage& message, const TString &workerhost, TWorkerPool* pool) {
    PROFILE_ACQUIRE(ID_FULL_STATUS_MSG_TIME)

    LOG("Processing full status from " << workerhost);

    Y_ASSERT(size_t(Targets.end() - Targets.begin()) == message.TargetSize());

    TFullStatusMessage::TRepeatedTarget::const_iterator messageTarget = message.GetTarget().begin();
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target, ++messageTarget) {
        if (!(*target)->Type->HasHostname(workerhost)) {
            // for this worker, skip targets it doesn't know of
            continue;
        }

        (*target)->Revision.Up();

        //Y_ASSERT(worker != (*target)->Workers.end());
        int masterTaskCount = (*target)->Type->GetTaskCountByWorker(workerhost);
        int workerTaskCount = messageTarget->StatusSize();
        Y_VERIFY(masterTaskCount == workerTaskCount,
                "master and worker have different opinion on number of clusters: host %s target %s, master thinks %d worker thinks %d",
                workerhost.data(), (*target)->Name.data(), masterTaskCount, workerTaskCount);

        TMasterTarget::THostId hostId = (*target)->Type->GetHostId(workerhost);

        TTaskStatuses::TRepeatedStatus::const_iterator messageStatus = messageTarget->GetStatus().begin();
        for (TMasterTarget::TTaskIterator task = (*target)->TaskIteratorForHost(workerhost); task.Next(); ++messageStatus) {
            const TTaskState& fromState = task->Status.GetState();
            const TTaskState& toState = TTaskState::Make(messageStatus->GetState(), messageStatus->GetStateValue());

            (*target)->TargetStats.ChangeState(hostId, fromState, toState);

            (*target)->TargetStats.UpdateTimes(hostId, *messageStatus);

            task->Status = *messageStatus;
        }
    }

    PROFILE_ACQUIRE(ID_FULL_STATUS_MSG_POKES_TIME)
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        SchedulePokeUpdate(*target, pool);
    }
    PROFILE_RELEASE(ID_FULL_STATUS_MSG_POKES_TIME)

    DEBUGLOG("Processing full status OK");

    PROFILE_RELEASE(ID_FULL_STATUS_MSG_TIME)
}

void TMasterGraph::ProcessSingleStatus(TSingleStatusMessage& message, const TString &workerhost, TWorkerPool* pool) {
    PROFILE_ACQUIRE(ID_SINGLE_STATUS_MSG_TIME)

    TTargetsList::iterator target = Targets.find(message.GetName());
    DEBUGLOG("Processing single status from"
            << " host " << workerhost
            << " target " << message.GetName()
            << " task " << message.GetTask()
            );
    if (target == Targets.end())
        ythrow yexception() << "status for unknown target received: " << message.GetName();

    (*target)->Revision.Up();

    TMasterTarget::THostIdSafe hostId = (*target)->Type->GetHostIdSafe(workerhost);
    TTaskStatus& ost = (*target)->GetTaskStatusByWorkerIdAndLocalTaskN(hostId, message.GetTask()).Status;

    const TTaskState& fromState = ost.GetState();
    const TTaskState& toState = TTaskState::Make(message.GetStatus().GetState(), message.GetStatus().GetStateValue());

    if (HasObservers && toState == TS_FAILED) {
        PendingFailureNotificationsDispatch.insert(*target);
    }

    (*target)->TargetStats.ChangeState(hostId.Id, fromState, toState);
    (*target)->TargetStats.UpdateTimes(hostId.Id, message.GetStatus());

    ost = message.GetStatus();

    PROFILE_ACQUIRE(ID_SINGLE_STATUS_MSG_POKES_TIME)
    for (TMasterTarget::TDependsList::iterator follower = (*target)->Followers.begin(); follower != (*target)->Followers.end(); ++follower) {
        SchedulePokeUpdate(follower->GetTarget(), pool);
    }
    PROFILE_RELEASE(ID_SINGLE_STATUS_MSG_POKES_TIME)

    PROFILE_RELEASE(ID_SINGLE_STATUS_MSG_TIME)
}

void TMasterGraph::ProcessThinStatus(TThinStatusMessage& message, const TString &workerhost, TWorkerPool* pool) {
    PROFILE_ACQUIRE(ID_THIN_STATUS_MSG_TIME)

    TTargetsList::const_iterator target = Targets.find(message.GetName());

    if (target == Targets.end())
        ythrow yexception() << "thin status for unknown target " << message.GetName() << " received from the worker " << workerhost;

    if ((*target)->Type->GetTaskCountByWorker(workerhost) != message.StateSize())
        ythrow yexception() << "bad sized thin status for target " << message.GetName() << " received from the worker " << workerhost;

    (*target)->Revision.Up();

    TMasterTarget::THostId hostId = (*target)->Type->GetHostId(workerhost);

    bool allAreNowIdle = true;
    int state = 0;
    for (TMasterTarget::TTaskIterator task = (*target)->TaskIteratorForHost(workerhost); task.Next(); ++state) {
        const NProto::TThinTaskStatus* const status = (static_cast<size_t>(state) < message.StatusSize()) ? &message.GetStatus(state) : nullptr;

        const TTaskState& fromState = task->Status.GetState();
        const TTaskState& toState = status
            ? TTaskState::Make(status->GetState(), status->GetStateValue())
            : TTaskState::Make(message.GetState(state));

        if (toState != TS_IDLE) {
            allAreNowIdle = false;
        }

        (*target)->TargetStats.ChangeState(hostId, fromState, toState);

        task->Status.SetState(toState);
        task->Status.SetLastChanged(status ? status->GetLastChanged() : 0);
    }

    PROFILE_ACQUIRE(ID_THIN_STATUS_MSG_POKES_TIME)
    if (!allAreNowIdle) { // For faster reset
        SchedulePokeUpdate(*target, pool);
        for (TMasterTarget::TDependsList::iterator follower = (*target)->Followers.begin(); follower != (*target)->Followers.end(); ++follower) {
            SchedulePokeUpdate(follower->GetTarget(), pool);
        }
    }
    PROFILE_RELEASE(ID_THIN_STATUS_MSG_POKES_TIME)

    PROFILE_RELEASE(ID_THIN_STATUS_MSG_TIME)
}

void TMasterGraph::PropagateThinStatus(TString const& targetName, const TTaskState& state, TWorkerPool* pool) {
    LOG("THIN Propagating: " << targetName << ", ");

    TMasterGraph::TTargetsList::iterator target = Targets.find(targetName);

    for (TMasterTarget::TTaskIterator task = (*target)->TaskIterator(); task.Next(); ) {
        TMasterTarget::THostIdSafe hostId = task.GetPathId().at(TTargetTypeParameters::HOST_LEVEL_ID - 1);
        (*target)->TargetStats.ChangeState(hostId.Id, task->Status.GetState(), state);
        task->Status.SetState(state);
    }

    SchedulePokeUpdate(*target, pool);
    for (TMasterTarget::TDependsList::iterator follower = (*target)->Followers.begin();
            follower != (*target)->Followers.end(); ++follower) {
        SchedulePokeUpdate(follower->GetTarget(), pool);
    }
}


void TMasterGraph::ExportConfig(const TString& worker, TConfigMessage* msg) const {
    LOG("Exporting config");

    TConfigMessage* graph = Config.Get();
    if (!graph)
        ythrow yexception() << "no config loaded";

    // Copy graph from loaded one
    msg->CopyFrom(*graph);

    msg->SetWorkerName(worker);

    // Fill lists
    ListManager->ExportLists(worker, msg);

    // Fill targets for this worker
    for (TTargetTypesList::const_iterator type = Types.begin(); type != Types.end(); ++type) {
        if ((*type)->HasHostname(worker)) {
            TConfigMessage::TThisWorkerTargetType* thisWorkerTargetType = msg->AddThisWorkerTargetTypes();
            thisWorkerTargetType->SetName((*type)->GetName());
            thisWorkerTargetType->SetChecksum((*type)->ChecksumForWorker(worker));
        }
    }

    // Fill target types for this worker
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        if ((*target)->Type->HasHostname(worker)) {
            TConfigMessage::TThisWorkerTarget* thisWorkerTarget = msg->AddThisWorkerTargets();
            thisWorkerTarget->SetName((*target)->Name);
            thisWorkerTarget->SetHasCrossnodeDepends((*target)->HasCrossnodeDepends);

            // TODO: useless
            for (TMasterTarget::TDependsList::const_iterator depend = (*target)->Depends.begin();
                    depend != (*target)->Depends.end(); ++depend)
            {
                TConfigMessage::TThisWorkerTarget::TDepend* msgDepend = thisWorkerTarget->AddDepends();
                msgDepend->SetDependName(depend->GetRealTarget()->GetName());
            }
        }
    }

    DEBUGLOG("Exporting config OK");
}

void TMasterGraph::LoadConfig(const TMasterConfigSource& configSource, IOutputStream* dumpStream) {
    LOG("Loading config");

    THolder<TConfigMessage> newConfig(::ParseMasterConfig(configSource, HostName(), MasterOptions.HTTPPort));
    if (dumpStream) {
        dumpStream->Write(newConfig->GetConfig().data(), newConfig->GetConfig().size());
        dumpStream->Flush();
    }

    DEBUGLOG("Loading config OK");

    LOG("Parsing config");

    TaggedSubgraphs.clear();
    ParseConfig(newConfig.Get());
    Config.Swap(newConfig);

    DEBUGLOG("Checking internal state");

    CheckState();

    DEBUGLOG("Processing subgraphs for cron entries");
    GatherCronSubgraphs();

    DEBUGLOG("Parsing config OK");

    DEBUGLOG("Loading master variables");
    try {
        LoadVariables();
    } catch (const yexception& e) {
        LOG("No master variables: " << e.what());
    }
    DEBUGLOG("Loading master variables OK");
}

void TMasterGraph::ConfigureWorkers(const TLockableHandle<TWorkerPool>::TReloadScriptWriteLock& pool) {
    // Set new workers
    try {
        LOG("Creating workers list");

        TVector<TString> newWorkers;

        for (TTargetTypesList::const_iterator type = Types.begin(); type != Types.end(); ++type)
            newWorkers.insert(newWorkers.end(), (*type)->GetHosts().begin(), (*type)->GetHosts().end());

        pool->SetWorkers(newWorkers);

        Timestamp = TInstant::Now();
        ReloadScriptState.ReloadWasStarted(newWorkers.size());

        DEBUGLOG("Creating workers list OK");
    } catch (const yexception& e) {
        ERRORLOG("Creating workers list FAILED: " << e.what());

        pool->Reset();

        Timestamp = TInstant::Now();
        ReloadScriptState.ReloadWasStarted(0);

        throw;
    }
}

static const TTaskState ReadyToRunStates[] = {
    TS_SUCCESS,
};

static const TTaskState AfterResetStates[] = {
    TS_IDLE,
};

static const TTaskState AfterRunStates[] = {
    TS_RUNNING,
    TS_PENDING,
    TS_SUCCESS,
    TS_FAILED,
    TS_DEPFAILED,
    TS_READY,
};

static const TTaskState FailureStates[] = {
    TS_FAILED,
    TS_DEPFAILED,
};

bool TMasterGraph::EveryTaskHasStatus(const TCronState::TSubgraph& subgraph, const TTaskState* states, size_t statesCount) const {
    typedef TCronState::TSubgraph::TTargets TTargets;
    typedef TCronState::TSubgraphTargetWithTasks::TTasks TTasks;

    for (TTargets::const_iterator target = subgraph.GetTargets().begin(); target != subgraph.GetTargets().end(); ++target) {
        const TTasks& subgraphTasks = target->GetTasks();

        for (TTasks::const_iterator task = subgraphTasks.begin(); task != subgraphTasks.end(); ++task) {
            const TMasterTarget::TTaskList& allTasks = target->GetTarget()->GetTasks();

            bool bad = true;

            for (size_t s = 0; s < statesCount; ++s) {
                if (allTasks.At(*task).Status.GetState() == states[s]) {
                    bad = false;
                    break;
                }
            }

            if (bad) {
                return false;
            }
        }
    }

    return true;
}

bool TMasterGraph::SomeTaskHasStatus(const TCronState::TSubgraph& subgraph, const TTaskState* states, size_t statesCount) const {
    typedef TCronState::TSubgraph::TTargets TTargets;
    typedef TCronState::TSubgraphTargetWithTasks::TTasks TTasks;

    for (TTargets::const_iterator target = subgraph.GetTargets().begin(); target != subgraph.GetTargets().end(); ++target) {
        const TTasks& subgraphTasks = target->GetTasks();

        for (TTasks::const_iterator task = subgraphTasks.begin(); task != subgraphTasks.end(); ++task) {
            const TMasterTarget::TTaskList& allTasks = target->GetTarget()->GetTasks();

            for (size_t s = 0; s < statesCount; ++s)
                if (allTasks.At(*task).Status.GetState() == states[s])
                    return true;
        }
    }

    return false;
}

void TMasterGraph::CmdWholePathOnWorkers(const TMasterTarget& target, const TWorker& onWorker, const THashSet<TString>& subgraphHosts,
        const TLockableHandle<TWorkerPool>::TWriteLock& pool, ui32 cmd) {
    cmd |= TCommandMessage::CF_RECURSIVE_UP;

    {
        TMasterTarget::TTraversalGuard guard;
        target.CheckConditions(cmd, guard, pool.Get());
    }

    TCommandMessage2 msg(target.Name, cmd);

    for (TMasterTarget::TConstTaskIterator it = target.TaskIteratorForHost(onWorker.GetHost()); it.Next(); )
        msg.AddTask(it.GetN().GetN());

    for (THashSet<TString>::const_iterator host = subgraphHosts.begin(); host != subgraphHosts.end(); host++)
        pool->EnqueueMessageToWorker(*host, msg);
}

void TMasterGraph::CmdWholePathOnAllWorkers(const TMasterTarget& target, const TWorker& onWorker,
        const TLockableHandle<TWorkerPool>::TWriteLock& pool, ui32 cmd) {
    cmd |= TCommandMessage::CF_RECURSIVE_UP;

    {
        TMasterTarget::TTraversalGuard guard;
        target.CheckConditions(cmd, guard, pool.Get());
    }

    TCommandMessage2 msg(target.Name, cmd);

    for (TMasterTarget::TConstTaskIterator it = target.TaskIteratorForHost(onWorker.GetHost()); it.Next(); )
        msg.AddTask(it.GetN().GetN());

    pool->EnqueueMessageAll(msg);
}

static const TDuration RETRY_RESET_OR_START_INTERVAL = TDuration::Minutes(5);
static const int MAX_RETRIES = 3;
static const TDuration MAX_TIME_WAIT_FOR_RESET_OR_START = RETRY_RESET_OR_START_INTERVAL * (MAX_RETRIES + 1);

void TMasterGraph::ProcessFailedCron(const TMasterTarget& target, const TString& workerName, TCronState& rsct, bool& stateChanged) {
    TString what = rsct.GetState() == NProto::RS_WAITING_FOR_RESET ? "RESET" : "RUN";
    ERRORLOG("Cron (restart_on_success): it took too long for target " << target.GetName() << " (worker " << workerName << ") to " << what << ". "
             "Something bad has happened (host is unavailable for long time or user has done something via web interface). "
             "Setting cron restart state to INITIAL hoping everything would be fine..");
    rsct.SetStateResetRetries(NProto::RS_INITIAL);
    rsct.SetFailed(true);
    stateChanged = true;
}

void TMasterGraph::GatherCronSubgraphs() {
    for (TTargetsList::const_iterator t = Targets.begin(); t != Targets.end(); ++t) {
        if ((*t)->GetRestartOnSuccessSchedule().Get() != nullptr || (*t)->GetRetryOnFailureSchedule().Get() != nullptr) {
            const TVector<TString>& hosts = (*t)->Type->GetHosts();
            for (TVector<TString>::const_iterator w = hosts.begin(); w != hosts.end(); ++w) {
                TCronState& cronState = (*t)->CronStateForHost(*w);
                cronState.GatherSubgraph(**t, *w);
            }
        }
    }
}

void TMasterGraph::RetryOnFailure(const TLockableHandle<TWorkerPool>::TWriteLock& pool, TInstant now, TTarget& target) {
    const TCronEntry *retryCron = target.GetRetryOnFailureSchedule().Get();
    bool retryCronTriggered = (*retryCron)(now);

    for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin();
         worker != pool->GetWorkers().end(); ++worker) {
        if (!target.Type->HasHostname((*worker)->GetHost()))
            continue;

        TCronState& workerCronState = target.CronStateForHost((*worker)->GetHost());

        if (retryCronTriggered && SomeTaskHasStatus(workerCronState.GetSubgraph(), FailureStates, Y_ARRAY_SIZE(FailureStates))) {
            LOG("Cron (retry_on_failure): some tasks have failed - doing RUN whole path from target " << target.GetName() << " (worker " << (*worker)->GetHost() << ")..");
            CmdWholePathOnWorkers(target, **worker, workerCronState.GetSubgraphHosts(), pool, TCommandMessage::CF_RUN | TCommandMessage::CF_RETRY);
        }
    }
}

/*
 * checkTriggeredCron is needed as we want to call this method more often than one minute (to make cycle reset-wait-run faster and thus
 * allow for fast and small task with restart_on_sucess='* * * * *' to be executed every minute) but we can't process trigger cron expression
 * more often than every minute
 */
void TMasterGraph::ProcessCronEntries(const TLockableHandle<TWorkerPool>::TWriteLock& pool, TInstant now, bool checkTriggeredCron) {
    DEBUGLOG("Processing cron entries");

    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        const TCronEntry *restartCron = (*target)->GetRestartOnSuccessSchedule().Get();
        if ((*target)->GetRestartOnSuccessEnabled() && restartCron && restartCron->IsCronFormat()) {
            bool restartCronTriggered = checkTriggeredCron && (*restartCron)(now);

            for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker) {
                if (!(*target)->Type->HasHostname((*worker)->GetHost()))
                    continue;

                TCronState& workerCronState = (*target)->CronStateForHost((*worker)->GetHost());

                bool stateNeedToBeSavedToDisk = false;

                switch(workerCronState.GetState()) {
                    case NProto::RS_INITIAL:
                        if (restartCronTriggered
                            && (EveryTaskHasStatus(workerCronState.GetSubgraph(), ReadyToRunStates, Y_ARRAY_SIZE(ReadyToRunStates))
                                || (workerCronState.IsFailed() && EveryTaskHasStatus(workerCronState.GetSubgraph(),  AfterResetStates, Y_ARRAY_SIZE(AfterResetStates)))
                                )
                            )
                        {
                            LOG("Cron (restart_on_success): doing RESET whole path from target "
                                << (*target)->GetName()
                                << (workerCronState.IsFailed()? " (after CF)" : "")
                                << " (worker " << (*worker)->GetHost() << ").."
                            );
                            CmdWholePathOnWorkers(**target, **worker, workerCronState.GetSubgraphHosts(),
                                    pool, TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE);
                            workerCronState.SetStateResetRetries(NProto::RS_WAITING_FOR_RESET);
                            workerCronState.SetFailed(false);
                            stateNeedToBeSavedToDisk = true;
                        }
                        break;
                    case NProto::RS_WAITING_FOR_RESET:
                        if (EveryTaskHasStatus(workerCronState.GetSubgraph(), AfterResetStates, Y_ARRAY_SIZE(AfterResetStates))) {
                            LOG("Cron (restart_on_success): doing RUN whole path from target " << (*target)->GetName() << " (worker " << (*worker)->GetHost() << ")..");
                            CmdWholePathOnWorkers(**target, **worker, workerCronState.GetSubgraphHosts(),
                                    pool, TCommandMessage::CF_RUN | TCommandMessage::CF_IDEMPOTENT);
                            workerCronState.SetStateResetRetries(NProto::RS_WAITING_FOR_STARTED);
                            stateNeedToBeSavedToDisk = true;
                        } else if ((now - workerCronState.GetChangedTime()) > RETRY_RESET_OR_START_INTERVAL * (workerCronState.GetNumberOfCommandRetries() + 1)
                                && workerCronState.GetNumberOfCommandRetries() < MAX_RETRIES)
                        {
                            LOG("Cron (restart_on_success): it is taking too long to RESET whole path from target " << (*target)->GetName() << " (worker " << (*worker)->GetHost() << "). Sending another RESET command..");
                            CmdWholePathOnWorkers(**target, **worker, workerCronState.GetSubgraphHosts(),
                                    pool, TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE);
                            workerCronState.CommandWasRetried();
                        } else if ((now - workerCronState.GetChangedTime()) > MAX_TIME_WAIT_FOR_RESET_OR_START) { // 2 cases here: 1) all retries didn't help 2) user did something via web interface (send run command for task that was reseted by cron)
                            ProcessFailedCron(**target, (*worker)->GetHost(), workerCronState, stateNeedToBeSavedToDisk);
                        }
                        break;
                    case NProto::RS_WAITING_FOR_STARTED:
                        if (EveryTaskHasStatus(workerCronState.GetSubgraph(), AfterRunStates, Y_ARRAY_SIZE(AfterRunStates))) {
                            workerCronState.SetStateResetRetries(NProto::RS_INITIAL);
                            stateNeedToBeSavedToDisk = true;
                        } else if ((now - workerCronState.GetChangedTime()) > RETRY_RESET_OR_START_INTERVAL * (workerCronState.GetNumberOfCommandRetries() + 1)
                                && workerCronState.GetNumberOfCommandRetries() < MAX_RETRIES)
                        {
                            LOG("Cron (restart_on_success): it is taking too long to RUN whole path from target " << (*target)->GetName() << " (worker " << (*worker)->GetHost() << "). Sending another RUN command..");
                            CmdWholePathOnWorkers(**target, **worker, workerCronState.GetSubgraphHosts(),
                                    pool, TCommandMessage::CF_RUN | TCommandMessage::CF_IDEMPOTENT);
                            workerCronState.CommandWasRetried();
                        } else if ((now - workerCronState.GetChangedTime()) > MAX_TIME_WAIT_FOR_RESET_OR_START) { // again: 2 cases here, see above
                            ProcessFailedCron(**target, (*worker)->GetHost(), workerCronState, stateNeedToBeSavedToDisk);
                        }
                        break;
                    default:
                        Y_FAIL("Impossible state");
                }

                if (stateNeedToBeSavedToDisk) { // when we were saving state unconditionally we have several seconds hangs of this code on
                    // server where disk load was very high (CLUSTERMASTER-55)
                    (*target)->SaveState();
                }
            }
        }

        const TCronEntry *retryCron = (*target)->GetRetryOnFailureSchedule().Get();
        if ((*target)->GetRetryOnFailureEnabled() && retryCron && retryCron->IsCronFormat() && checkTriggeredCron)
            RetryOnFailure(pool, now, **target);
    }

    DEBUGLOG("Finished processing cron entries");
}

void TMasterGraph::ProcessCronEntriesPerSecond(const TLockableHandle<TWorkerPool>::TWriteLock& pool, TInstant now) {
    DEBUGLOG("Processing cron entries per second");

    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        const TCronEntry* restartCron = (*target)->GetRestartOnSuccessSchedule().Get();
        if ((*target)->GetRestartOnSuccessEnabled() && restartCron && !restartCron->IsCronFormat()) {
            bool restartCronTriggered = (*restartCron)(now);

            for (TWorkerPool::TWorkersList::const_iterator worker = pool->GetWorkers().begin(); worker != pool->GetWorkers().end(); ++worker) {
                if (!(*target)->Type->HasHostname((*worker)->GetHost()))
                    continue;

                TCronState& workerCronState = (*target)->CronStateForHost((*worker)->GetHost());

                if (restartCronTriggered && EveryTaskHasStatus(workerCronState.GetSubgraph(), ReadyToRunStates, Y_ARRAY_SIZE(ReadyToRunStates))) {
                    LOG("Cron (restart_on_success): doing RESET whole path from target " << (*target)->GetName() << " (worker " << (*worker)->GetHost() << ")..");
                    workerCronState.SetFailed(false);
                    LOG("Cron (restart_on_success): doing RUN whole path from target " << (*target)->GetName() << " (worker " << (*worker)->GetHost() << ")..");

                    auto flags = NCA::GetFlags("forced-run");
                    CmdWholePathOnWorkers(**target, **worker, workerCronState.GetSubgraphHosts(), pool, flags);
                }
            }
        }

        const TCronEntry* retryCron = (*target)->GetRetryOnFailureSchedule().Get();
        if ((*target)->GetRetryOnFailureEnabled() && retryCron && !retryCron->IsCronFormat()) {
            RetryOnFailure(pool, now, **target);
        }
    }

    DEBUGLOG("Finished processing cron entries");
}

TString PokeStatisticsStr(int pendingPokeUpdatesSize, int performedPokeUpdates) {
    TString result;
    TStringOutput out(result);
    out << "Number of targets scheduled for poke status update is " << pendingPokeUpdatesSize << ", performed sync poke updates is " << performedPokeUpdates;
    return result;
}

void TMasterGraph::ProcessPokes(const TLockableHandle<TWorkerPool>::TWriteLock& pool) {
    if (!ReloadScriptState.IsInProgress()) {
        DEBUGLOG("Processing pokes. " << PokeStatisticsStr(PendingPokeUpdates.size(), PerformedPokeUpdates));

        for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
            if (PendingPokeUpdates.find(*target) != PendingPokeUpdates.end())
                (*target)->UpdatePokeState();
            (*target)->TryPoke(pool.Get());
        }

        PendingPokeUpdates.clear();
        PerformedPokeUpdates = 0;

        DEBUGLOG("Finished processing pokes");
    } else {
        DEBUGLOG("Reload script is in progress. Skipping processing pokes. " << PokeStatisticsStr(PendingPokeUpdates.size(), PerformedPokeUpdates));
    }
}

void TMasterGraph::SetAllTaskState(const TTaskState& state) {
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        (*target)->SetAllTaskState(state);
    }
}

void TMasterGraph::SchedulePokeUpdate(TMasterTarget* target, TWorkerPool* pool) {
    if (!ReloadScriptState.IsInProgress() && PerformedPokeUpdates < MasterOptions.MaxPokesPerHeartbeat) {
        if (target->UpdatePokeState())
            target->TryPoke(pool);

        PerformedPokeUpdates++;

        if (PerformedPokeUpdates == MasterOptions.MaxPokesPerHeartbeat)
            DEBUGLOG("Poke limit per heartbeat (" << MasterOptions.MaxPokesPerHeartbeat << ") exceeded; delaying following pokes until heartbeat...");
    } else {
        PendingPokeUpdates.insert(target);
    }
}

void TMasterGraph::MaintainReloadScriptState(WorkersStat workersStat) {
    if (ReloadScriptState.IsInProgressIgnoreDeadline() && ((workersStat.ActivePercent() >= 95) && (workersStat.InactiveCount() <= 7))) {
        ReloadScriptState.ReloadWasEnded();
        DEBUGLOG("We had worker quorum. Thus we consider that reload script was ended.");
    }
}


void TMasterGraph::UpdateTargetsStats() {
    DEBUGLOG("Updating targets statistics");

    int updatedFor = 0;
    for (TMasterGraph::TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        TTargetByWorkerStats& stats = (*target)->TargetStats;

        if ((*target)->GetRevision() <= stats.RevisionWhenUpdated) // no need in update - same revision
            continue;

        TStatsCalculator targetCalc;
        for (TIdForString::TIterator worker = (*target)->Type->GetHostList().Iterator(); worker.Next(); ) {
            TMasterTarget::THostId workerId = worker->Id;

            TStatsCalculator calculator;
            for (TMasterTarget::TConstTaskIterator task = (*target)->TaskIteratorForHost(workerId); task.Next(); ) {
                targetCalc.Add(task->Status);
                calculator.Add(task->Status);
            }
            calculator.Finalize();

            TStats& workerStats = stats.ByWorker.at(workerId);

            workerStats.Resources = calculator.GetResources();
            workerStats.Durations = calculator.GetDurations();
            workerStats.AggregateDurations = calculator.GetAggregateDurations();
        }
        targetCalc.Finalize();

        TStats& targetStats = stats.Whole;

        targetStats.Resources = targetCalc.GetResources();
        targetStats.Durations = targetCalc.GetDurations();
        targetStats.AggregateDurations = targetCalc.GetAggregateDurations();

        stats.RevisionWhenUpdated = (*target)->GetRevision();

        updatedFor++;
    }

    DEBUGLOG("Statistics was updated for " << updatedFor << " targets of " << Targets.size());
}

void NAsyncJobs::TGenerateGraphImageJob::Process(void*) {
    try {
        LOG("Generating graph image");

        TInstant timestamp;

        TString dumpedGraphviz;
        ui64 dumpedGraphvizCrc = 0;
        TString graphImage;

        bool hasGraphImage = false;

        do {
            TLockableHandle<TMasterGraph>::TReadLock graph(Graph);

            timestamp = graph->GetTimestamp();
            hasGraphImage = graph->GraphImage.Get();

            dumpedGraphviz = graph->DumpGraphviz();
            dumpedGraphvizCrc = crc64(dumpedGraphviz.data(), dumpedGraphviz.size());

            if (graph->GraphImage.Get() && graph->GraphvizCrc == dumpedGraphvizCrc)
                return;
        } while (false);

        if (hasGraphImage) {
            TLockableHandle<TMasterGraph>::TWriteLock graph(Graph);

            if (timestamp != graph->GetTimestamp())
                return;

            graph->GraphImage.Destroy();
        }

        do {
            TString random_token = ToString(MicroSeconds()) + ToString(RandomNumber<ui64>());
            TTempFile graphDataFile(TString("/tmp/clustermaster-graph-data-") + random_token);
            TTempFile graphImageFile(TString("/tmp/clustermaster-graph-image-") + random_token);

            TUnbufferedFileOutput(graphDataFile.Name()).Write(dumpedGraphviz.data(), dumpedGraphviz.size());

            TFileHandle in(graphDataFile.Name(), OpenExisting | RdOnly);
            TFileHandle out(graphImageFile.Name(), CreateAlways | WrOnly | ARW);

            pid_t childPid = fork();

            if (childPid == -1)
                return;

            if (childPid == 0) {
                // Child; no memory allocation after this point!
                sigset_t mask;
                SigFillSet(&mask);
                SigProcMask(SIG_UNBLOCK, &mask, nullptr);

                TFileHandle stdinHandle(0);
                TFileHandle stdoutHandle(1);

                if (!stdinHandle.LinkTo(in)) {
                    ERRORLOG("Graph: cannot link stdin handle");
                    _exit(1);
                }

                if (!stdoutHandle.LinkTo(out)) {
                    ERRORLOG("Graph: cannot link stdout handle");
                    _exit(1);
                }

                stdinHandle.Release();
                stdoutHandle.Release();

                for (int fd = 3; fd < getdtablesize(); ++fd)
                    close(fd);

                if (MasterOptions.GraphvizIterationsLimit != 0) {
                    TStringStream graphVizZOpt;
                    graphVizZOpt << "-z" << MasterOptions.GraphvizIterationsLimit;
                    execlp("dot", "dot", "-Tsvg", graphVizZOpt.Str().c_str(), NULL);
                    // Passing the -z flag here to limit the number of iterations with 300000
                    // This only works with a patch for graphviz (located in arcadia repository: junk/hindsight/graphviz)
                } else {
                    execlp("dot", "dot", "-Tsvg", NULL);
                }

                ERRORLOG("Graph: cannot execute dot");

                _exit(1);
            }

            int ret = 0;

            while (waitpid(childPid, &ret, 0) == -1 && errno == EINTR)
                {}

            if (ret != 0)
                ythrow yexception() << "Could not build graph (nonzero exit code from child process)";

            TString graphXsltData;
            GetStaticFile("/graph.xslt", graphXsltData);
            graphImage = TXslTransform(graphXsltData.data())(TUnbufferedFileInput(graphImageFile.Name()).ReadAll());
        } while (false);

        TLockableHandle<TMasterGraph>::TWriteLock graph(Graph);

        if (timestamp != graph->GetTimestamp()) {
            DEBUGLOG("Generating graph image canceled (script was updated?)");
            return;
        }

        graph->GraphImage.Reset(new TString(graphImage));
        graph->GraphvizCrc = dumpedGraphvizCrc;
        DEBUGLOG("Generating graph image OK");
    } catch (...) {
        TLockableHandle<TMasterGraph>::TWriteLock graph(Graph);

        graph->GraphImage.Reset(nullptr);
        graph->GraphvizCrc = 0;
        ERRORLOG("Generating graph image FAILED: " << CurrentExceptionMessage());
    }
}

typedef NAsyncJobs::TSendMailJob::TTaskWithWorker TTaskWithWorker;

TVector<TTaskWithWorker> TasksWithWorkersByMask(const TMasterTarget& target, const NGatherSubgraph::TResultForTarget& resultForTarget) {
    TVector<TTaskWithWorker> result;
    for (TMasterTarget::TConstTaskIterator task = target.TaskIterator(); task.Next(); ) {
        if (resultForTarget.Mask.At(task.GetN()) == true && resultForTarget.IsNotSkipped()) {
            int taskN = task.GetN().GetN();
            TString worker = task.GetPath()[0];
            result.push_back(TTaskWithWorker(taskN, worker));
        }
    }
    return result;
}

TVector<TTaskWithWorker> TasksWithWorkersByMask(const TMasterTarget& target, const NGatherSubgraph::TMask& mask) {
    NGatherSubgraph::TResultForTarget resultForTarget(mask);
    resultForTarget.Skipped = NGatherSubgraph::TResultForTarget::SS_NOT_SKIPPED;
    return TasksWithWorkersByMask(target, resultForTarget);
}

void TMasterGraph::DispatchFailureNotifications() {
    if (!ReloadScriptState.IsInProgress()) {
        DEBUGLOG("Dispatching failure emails");

        for (TargetsSet::const_iterator rootTarget = PendingFailureNotificationsDispatch.begin();
                rootTarget != PendingFailureNotificationsDispatch.end(); ++rootTarget)
        {
            bool hasFailedTasks = false;
            NGatherSubgraph::TMask failedMask(&(*rootTarget)->Type->GetParameters());
            for (TMasterTarget::TConstTaskIterator task = (*rootTarget)->TaskIterator(); task.Next(); ) {
                if (task->Status.GetState() == TS_FAILED) {
                    hasFailedTasks = true;
                    failedMask.At(task.GetN()) = true;
                }
            }
            if (!hasFailedTasks) // Nothing to do (possible if target was already reseted - but this should be rare case)
                continue;

            // Gather subgraph to find out child targets with tasks (if child target have mailto
            // we need to notify subscribers that one of targets dependencies failed)
            NGatherSubgraph::TResult<TMasterTarget> result;
            NGatherSubgraph::GatherSubgraph<TMasterGraphTypes, TParamsByTypeOrdinary, TMasterTargetEdgeInclusivePredicate>
                (**rootTarget, failedMask, NGatherSubgraph::M_DEPFAIL, &result);

            // Send emails
            typedef NAsyncJobs::TSendMailJob::TTargetWithTasks TTargetWithTasks;

            // 1) About root target failure
            const TVector<TTaskWithWorker>& rootTargetTasks = TasksWithWorkersByMask(**rootTarget, failedMask);
            if ((*rootTarget)->Recipients.Get() != nullptr) {
                const TRecipients* mailRecipients = (*rootTarget)->Recipients->GetMailRecipients();
                if (mailRecipients != nullptr) {
                    TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                    AsyncMailJobs().Add(THolder(new NAsyncJobs::TSendMailJob(NAsyncJobs::TSendMailJob::FAILURE, *mailRecipients,
                        TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks), TMaybe<TTargetWithTasks>(), *variables)));
                }
                const TRecipients* smsRecipients = (*rootTarget)->Recipients->GetSmsRecipients();
                if (smsRecipients != nullptr) {
                    TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                    AsyncSmsJobs().Add(THolder(new NAsyncJobs::TSendSmsJob(NAsyncJobs::TSendSmsJob::FAILURE, *smsRecipients,
                        TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks), TMaybe<TTargetWithTasks>(), *variables, TvmClient)));
                }
                const TRecipients* telegramRecipients = (*rootTarget)->Recipients->GetTelegramRecipients();
                if (telegramRecipients != nullptr) {
                    TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                    AsyncTelegramJobs().Add(THolder(new NAsyncJobs::TSendTelegramJob(NAsyncJobs::TSendTelegramJob::FAILURE, *telegramRecipients,
                        TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks), TMaybe<TTargetWithTasks>(), *variables, TelegramSecret)));
                }
                const TRecipients* jnsChannelRecipients = (*rootTarget)->Recipients->GetJNSChannelRecipients();
                if (jnsChannelRecipients != nullptr) {
                    TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                    AsyncJNSChannelJobs().Add(THolder(new NAsyncJobs::TSendJNSChannelJob(NAsyncJobs::TSendJNSChannelJob::FAILURE, *jnsChannelRecipients,
                        TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks), TMaybe<TTargetWithTasks>(), *variables, JNSSecret)));
                }
                const TRecipients* jugglerTags = (*rootTarget)->Recipients->GetJugglerEventTags();
                if (jugglerTags != nullptr && MasterOptions.EnableJuggler) {
                    TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                    AsyncJugglerJobs().Add(THolder(new NAsyncJobs::TPushJugglerEventJob(NAsyncJobs::TPushJugglerEventJob::FAILURE, *jugglerTags,
                        TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks), TMaybe<TTargetWithTasks>(), *variables)));
                }
            }

            // 2) About children targets
            typedef NGatherSubgraph::TResult<TMasterTarget>::TResultByTarget TResultByTarget;
            const TResultByTarget& resultByTarget = result.GetResultByTarget();
            for (TResultByTarget::const_iterator resultByTargetIterator = resultByTarget.begin(); resultByTargetIterator != resultByTarget.end(); ++resultByTargetIterator) {
                TMasterTarget* childTarget = const_cast<TMasterTarget*>(resultByTargetIterator->first);

                if (childTarget == *rootTarget) { // GatherSubgraph returns root target as well
                    continue;
                }

                const NGatherSubgraph::TResultForTarget* resultForTarget = resultByTargetIterator->second;

                if (childTarget->Recipients.Get() != nullptr) {
                    const TVector<TTaskWithWorker>& childTargetTasks = TasksWithWorkersByMask(*childTarget, *resultForTarget);
                    const TRecipients* mailRecipients = childTarget->Recipients->GetMailRecipients();
                    if (mailRecipients != nullptr) {
                        TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                        AsyncMailJobs().Add(THolder(new NAsyncJobs::TSendMailJob(
                            NAsyncJobs::TSendMailJob::DEPFAILURE,
                            *mailRecipients,
                            TTargetWithTasks(childTarget->GetName(), childTargetTasks),
                            TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks),
                            *variables)));
                    }
                    const TRecipients* smsRecipients = childTarget->Recipients->GetSmsRecipients();
                    if (smsRecipients != nullptr) {
                        TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                        AsyncSmsJobs().Add(THolder(new NAsyncJobs::TSendSmsJob(
                            NAsyncJobs::TSendSmsJob::DEPFAILURE,
                            *smsRecipients,
                            TTargetWithTasks(childTarget->GetName(), childTargetTasks),
                            TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks),
                            *variables,
                            TvmClient)));
                    }
                    const TRecipients* telegramRecipients = childTarget->Recipients->GetTelegramRecipients();
                    if (telegramRecipients != nullptr) {
                        TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                        AsyncTelegramJobs().Add(THolder(new NAsyncJobs::TSendTelegramJob(
                            NAsyncJobs::TSendTelegramJob::DEPFAILURE,
                            *telegramRecipients,
                            TTargetWithTasks(childTarget->GetName(), childTargetTasks),
                            TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks),
                            *variables,
                            TelegramSecret)));
                    }
                    const TRecipients* jnsChannelRecipients = childTarget->Recipients->GetJNSChannelRecipients();
                    if (jnsChannelRecipients != nullptr) {
                        TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                        AsyncJNSChannelJobs().Add(THolder(new NAsyncJobs::TSendJNSChannelJob(
                            NAsyncJobs::TSendJNSChannelJob::DEPFAILURE,
                            *jnsChannelRecipients,
                            TTargetWithTasks(childTarget->GetName(), childTargetTasks),
                            TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks),
                            *variables,
                            JNSSecret)));
                    }
                    const TRecipients* jugglerTags = childTarget->Recipients->GetJugglerEventTags();
                    if (jugglerTags != nullptr && MasterOptions.EnableJuggler) {
                        TLockableHandle<TVariablesMap>::TReadLock variables(Variables);
                        AsyncJugglerJobs().Add(
                            THolder(new NAsyncJobs::TPushJugglerEventJob(
                                NAsyncJobs::TPushJugglerEventJob::DEPFAILURE,
                                *jugglerTags,
                                TTargetWithTasks(childTarget->GetName(), childTargetTasks),
                                TTargetWithTasks((*rootTarget)->GetName(), rootTargetTasks),
                                *variables)));
                    }
                }
            }
        }

        PendingFailureNotificationsDispatch.clear();

        DEBUGLOG("Finished dispatching failure emails");
    } else {
        DEBUGLOG("Reload script is in progress. Skipping dispatching failure emails..");
    }
}

void TMasterGraph::ClearPendingPokeUpdatesAndFailureNotificationsDispatch() {
    PendingPokeUpdates.clear();
    PendingFailureNotificationsDispatch.clear();
}

const TTaggedSubgraphs& TMasterGraph::GetTaggedSubgraphs() const {
    return TaggedSubgraphs;
}

void TMasterGraph::AddTaggedSubgraph(const TString& tagName, const TTargetSubgraphPtr& subgraph) {
    auto it = TaggedSubgraphs.find(tagName);
    if (it == TaggedSubgraphs.end()) {
        TaggedSubgraphs.insert(std::make_pair(tagName, TTargetSubgraphList({subgraph})));
    }
    else {
        it->second.push_back(subgraph);
    }
}

const TFsPath TMasterGraph::GetVariablesFilePath() const {
    return VariablesFilePath(MasterOptions.VarDirPath);
}

NProto::TVariablesDump TMasterGraph::PackOnlyVariables(const TVariablesMap& vars) const {
    NProto::TVariablesDump result;
    for (const auto& [key, value] : vars) {
        NProto::TVariablesDump::TVariable* dst = result.AddVariable();
        dst->SetName(key);
        dst->SetValue(value);
    }
    return result;
}

NProto::TVariablesDump TMasterGraph::PackVariables() const {
    TLockableHandle<TVariablesMap>::TReadLock vars(Variables);
    NProto::TVariablesDump result = PackOnlyVariables(*vars);
    result.SetUpdateTimestamp(MilliSeconds());
    return result;
}

void TMasterGraph::SaveVariablesToYt(const NProto::TVariablesDump &protoState) const {
    TMaybe<TYtStorage>& storage = MasterOptions.YtStorage;
    if (storage) {
        storage->SaveProtoStateToYt(TWorkerVariables::VariablesYtKey, protoState);
    }
}

void TMasterGraph::SaveVariablesToDisk(const NProto::TVariablesDump &protoState) const {
    ::SaveProtoStateToDisk(GetVariablesFilePath(), protoState);
}

void TMasterGraph::SaveVariables() const {
    LOG("Saving variables");
    const NProto::TVariablesDump protoState = PackVariables();
    SaveVariablesToDisk(protoState);
    SaveVariablesToYt(protoState);
    LOG("Saved variables");
}

TMaybe<NProto::TVariablesDump> TMasterGraph::LoadVariablesFromYt() const {
    TMaybe<TYtStorage>& storage = MasterOptions.YtStorage;
    if (!storage) {
        return {};
    }
    return storage->LoadAndParseStateFromYt<NProto::TVariablesDump>(TWorkerVariables::VariablesYtKey);
}

TMaybe<NProto::TVariablesDump> TMasterGraph::LoadVariablesFromDisk() const {
    return ::LoadAndParseStateFromDisk<NProto::TVariablesDump>(GetVariablesFilePath());
}

TMaybe<NProto::TVariablesDump> TMasterGraph::LoadVariablesFromDiskDeprecated() const {
    if (MasterOptions.VarDirPath.empty()) {
        return {};
    }
    const TFsPath fsPath = MasterVariablesDeprecatedFilePath(MasterOptions.VarDirPath);
    const TString& path = fsPath.GetPath();
    if (!fsPath.Exists()) {
        DEBUGLOG("Cannot load variables from nonexisting file " << path);
        return {};
    }
    try {
        TFile file(path, OpenExisting | RdOnly | Seq);
        file.Flock(LOCK_SH);

        TUnbufferedFileInput in(file);
        TVariablesMap map;
        Load<TVariablesMap>(&in, map);
        NProto::TVariablesDump result = PackOnlyVariables(map);
        return MakeMaybe(std::move(result));
    } catch (const yexception& e) {
        NFs::Remove(path);
        ERRORLOG("Failed to load variables from " << path << ": " << e.what());
        return {};
    }
}

void TMasterGraph::LoadVariables() const {
    LOG("Loading variables");
    TVector<TMaybe<NProto::TVariablesDump>> candidates;
    candidates.emplace_back(LoadVariablesFromYt());
    candidates.emplace_back(LoadVariablesFromDisk());
    candidates.emplace_back(LoadVariablesFromDiskDeprecated());

    TMaybe<NProto::TVariablesDump> newest = ::SelectByUpdateTimestamp(candidates);

    if (newest.Empty()) {
        ERRORLOG("Failed to load variables from any source");
        return;
    }
    LOG("Loading dump of "
        << newest->VariableSize()
        << " variables with timestamp "
        << newest->GetUpdateTimestamp());
    TLockableHandle<TVariablesMap>::TWriteLock vars(Variables);
    vars->clear();
    for (const auto& var : newest->GetVariable()) {
        vars->insert({var.GetName(), var.GetValue()});
    }
    LOG("Loaded variables");
}
