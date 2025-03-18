#include "worker_target_graph.h"

#include "graph_change_watcher.h"
#include "log.h"
#include "master_multiplexor.h"
#include "messages.h"
#include "worker.h"

#include <tools/clustermaster/common/state_file.h>
#include <tools/clustermaster/common/thread_util.h>
#include <tools/clustermaster/common/util.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/xml/encode/encodexml.h>

#include <util/folder/path.h>
#include <util/generic/singleton.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/string/util.h>
#include <util/system/fs.h>
#include <util/system/sigset.h>

#include <deque>
#include <set>

#ifndef _win_
#   include <sys/wait.h>
#endif

namespace LogSubsystem {
class process {};
};

TWorkerGraph::TWorkerGraph(TWorkerListManager* l, TWorkerGlobals* workerGlobals, bool forTest)
    : TTargetGraphBase<TWorkerGraphTypes>(l, forTest)
    , WorkerGlobals(workerGlobals)
    , ResourceMonitor(workerGlobals, forTest)
    , LowFreeSpaceModeOn(false)
{
}

const TFsPath TWorkerGraph::GetVariablesFilePath() const {
    return VariablesFilePath(WorkerGlobals->VarDirPath);
}

TMaybe<NProto::TVariablesDump> TWorkerGraph::LoadVariablesFromYt() const {
    TMaybe<TYtStorage>& storage = WorkerGlobals->YtStorage;
    if (!storage) {
        return {};
    }
    return storage->LoadAndParseStateFromYt<NProto::TVariablesDump>(TWorkerVariables::VariablesYtKey);
}

TMaybe<NProto::TVariablesDump> TWorkerGraph::LoadVariablesFromDisk() const {
    return ::LoadAndParseStateFromDisk<NProto::TVariablesDump>(GetVariablesFilePath());
}

void TWorkerGraph::LoadVariablesFromDiskDeprecated() {
    const TFsPath fsPath = WorkerVariablesDeprecatedFilePath(WorkerGlobals->VarDirPath);
    try {
        LOG("Loading variables (deprecated method) from disk");
        TUnbufferedFileInput variablesFile(fsPath.GetPath());
        Variables.ParseFromArcadiaStream(&variablesFile);
        DEBUGLOG("Loading variables (deprecated method) OK");
    } catch (const yexception& e) {
        ERRORLOG("Loading variables (deprecated method) from disk FAILED: " << e.what());
    }
}

void TWorkerGraph::LoadVariables() {
    TVector<TMaybe<NProto::TVariablesDump>> candidates;
    candidates.emplace_back(LoadVariablesFromYt());
    candidates.emplace_back(LoadVariablesFromDisk());

    TMaybe<NProto::TVariablesDump> newest = ::SelectByUpdateTimestamp(candidates);

    if (newest.Empty()) {
        LoadVariablesFromDiskDeprecated();
        return;
    }

    LOG("Loading dump of "
        << newest->VariableSize()
        << " variables with timestamp "
        << newest->GetUpdateTimestamp());

    Variables.Clear();
    for (const auto& var : newest->GetVariable()) {
        Variables.ApplyDefaultVariable(var.GetName(), var.GetValue());
    }
}

void TWorkerGraph::LoadState() {
    LOG("Loading state");
    LoadVariables();

    // Loads graph & predefined lists
    THolder<TUnbufferedFileInput> configFile;
    try {
        LOG("Loading config");
        configFile.Reset(new TUnbufferedFileInput(WorkerGlobals->ConfigPath));
        DEBUGLOG("Loading config OK");
    } catch (...) {
        throw TNoConfigException();
    }

    LOG("Parsing config");
    TConfigMessage config;
    config.ParseFromArcadiaStream(configFile.Get());
    DEBUGLOG("Parsing config OK");
    ImportConfig(&config);

    DEBUGLOG("Loaded state");
}

void TWorkerGraph::TryReadySomeTasks(TGraphChangeWatcher& watcher) {
    TGuard<TMutex> guard(Mutex);

    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target)
        (*target)->TryReadySomeTasks(watcher);
}

void TWorkerGraph::Poke(ui32 flags, const TString& target) {
    TGuard<TMutex> guard(Mutex);

    LOG("Poke (" << TPokeMessage::FormatText(target, flags) << ")");

    TTargetsList::const_iterator t = Targets.find(target);

    if (t == Targets.end())
        ythrow yexception() << "bad target poked: " << target;

    TGraphChangeWatcher watcher;

    if (flags == TPokeMessage::PF_READY)
        (*t)->PokeReadyAllTasks(watcher);

    watcher.Commit();

    DEBUGLOG("Poke (" << TPokeMessage::FormatText(target, flags) << ") OK");
}

void TWorkerGraph::Poke(ui32 flags, const TString& target, const TMultiPokeMessage::TRepeatedTasks& tasks) {
    TGuard<TMutex> guard(Mutex);

    LOG("MultiPoke (" << TMultiPokeMessage::FormatText(target, tasks, flags) << ")");

    TTargetsList::const_iterator t = Targets.find(target);

    if (t == Targets.end())
        ythrow yexception() << "bad target poked: " << target;

    TGraphChangeWatcher watcher;

    if (flags == TPokeMessage::PF_READY) {
        for (TMultiPokeMessage::TRepeatedTasks::const_iterator task = tasks.begin(); task != tasks.end(); ++task) {
            (*t)->PokeReady((*t)->Type->GetParameters().GetId(*task), watcher);
        }
    }

    watcher.Commit();

    DEBUGLOG("MultiPoke (" << TMultiPokeMessage::FormatText(target, tasks, flags) << ") OK");
}

NGatherSubgraph::TMask ProjectMaskOnLocalspace(const TWorkerTarget& t,
        const NGatherSubgraph::TMask& hyperspaceMask)
{
    NGatherSubgraph::TMask localspaceMask(&t.Type->GetParameters());

    for (unsigned i = 0; i < localspaceMask.Size(); ++i)
        localspaceMask.At(i) = hyperspaceMask.At(t.GetHyperspaceTaskIndex(i));

    return localspaceMask;
}

void TWorkerGraph::Depfail(TWorkerTarget* startTarget, const TTargetTypeParametersMap<bool>& startTargetMask,
        TGraphChangeWatcher& watcher, bool skipRootTarget)
{
    using namespace NGatherSubgraph;

    typedef TResult<TWorkerTarget> TResult;
    TResult subgraph;
    GatherSubgraph<TWorkerGraphTypes, TParamsByTypeHyperspace, TWorkerDepfailTargetEdgePredicate>
        (*startTarget, startTargetMask, M_DEPFAIL, &subgraph);

    TResult::TTargetList topoSortedTargets = subgraph.GetTopoSortedTargets();

    if(topoSortedTargets.empty()) { // Because of TWorkerDepfailTargetEdgePredicate::TargetIsOk/TWorkerDepfailTargetEdgePredicate::EdgeIsOk
            // GatherSubgraph could give us empty result
        return;
    }

    if (skipRootTarget)
        topoSortedTargets.pop_back();

    while (!topoSortedTargets.empty()) {
        TWorkerTarget* target = const_cast<TWorkerTarget*>(topoSortedTargets.back());

        if (target->Type->IsOnThisWorker()) {
            TResult::TResultByTarget::const_iterator resultForTargetIterator = subgraph.GetResultByTarget().find(target);
            Y_VERIFY(resultForTargetIterator != subgraph.GetResultByTarget().end());
            const TResultForTarget* resultForTarget = resultForTargetIterator->second;

            TMask mask = ProjectMaskOnLocalspace(*target, resultForTarget->Mask);

            for (unsigned task = 0; task != mask.Size(); ++task) {
                if (mask.At(task) == true) {
                    if (resultForTarget->IsNotSkipped() &&
                            target->GetTasks().At(task).GetState() == TS_PENDING)
                    {
                        TTargetTypeParameters::TId id(&target->Type->GetParameters(), task);
                        target->ChangeState(watcher, id, TS_DEPFAILED);
                    }
                }
            }
        }

        topoSortedTargets.pop_back();
    }
}

void TWorkerGraph::TaskStarted(TWorkerTarget* target, int nTask, pid_t pid) {
    auto it = RunningTasks.find(pid);
    if (it != RunningTasks.end()) {
        ERRORLOG("Already have pid " << pid << " in RunningTasks. Replace it with new task of " << target->Name);
        RunningTasks.erase(it);
    }

    RunningTasks.insert(std::make_pair(pid, TTaskHandle(target->Name, nTask)));
}

void TWorkerGraph::TryReapSomeTasks() {
#ifndef _win_
    TGuard<TMutex> guard(Mutex);

    pid_t childPid;
    int status;

    TGraphChangeWatcher watcher;
    while ((childPid = waitpid(-1, &status, WNOHANG | WSTOPPED | WCONTINUED)) > 0) {
        if (WIFEXITED(status)) {
            DEBUGLOG1(process, "child process " << childPid << " exited with status " << WEXITSTATUS(status));
            ProcessFinishedTask(childPid, watcher);
        } else if (WIFCONTINUED(status)) {
            DEBUGLOG1(process, "child process " << childPid << " was continued");
            ProcessContinuedTask(childPid, watcher);
        } else if (WIFSTOPPED(status)) {
            DEBUGLOG1(process, "child process " << childPid << " was stopped by signal " << WSTOPSIG(status));
            ProcessStoppedTask(childPid, watcher);
        } else if (WIFSIGNALED(status)) {
            DEBUGLOG1(process, "child process " << childPid << " was terminated by signal " << WTERMSIG(status) << (WCOREDUMP(status) ? " (core dumped)" : ""));
            ProcessFinishedTask(childPid, watcher);
        } else {
            Y_FAIL("Impossible status=%d", status);
        }
    }
    int saved_errno = errno;
    watcher.Commit();
    errno = saved_errno;

    if (childPid == -1 && LastSystemError() != ECHILD)
        ythrow yexception() << "waitpid failed: " << LastSystemErrorText();
#endif
}

void TWorkerGraph::ProcessFinishedTask(pid_t childPid, TGraphChangeWatcher &watcher) {
    TPidMap::iterator finishedProcess = RunningTasks.find(childPid);
    if (finishedProcess == RunningTasks.end()) {
        // Just ignore unknown childs - they may be spawned from
        // http server thread
        LOG("Skipped processing unknown finished task with pid " << childPid);
        return;
    }

    TTargetsList::const_iterator finishedTarget = Targets.find((finishedProcess->second).Target);
    if (finishedTarget == Targets.end()) {
        // Orphan task finished; forget it?
        LOG("Skipped processing orphan finished task with pid " << childPid);
        RunningTasks.erase(finishedProcess);
        return;
    }

    TTargetTypeParameters::TId taskId = (*finishedTarget)->Type->GetParameters().GetId(finishedProcess->second.NTask);
    (*finishedTarget)->Finished(taskId, watcher);

    RunningTasks.erase(finishedProcess);
}

void TWorkerGraph::ProcessStoppedTask(pid_t childPid, TGraphChangeWatcher &watcher) {
    TPidMap::iterator stoppedProcess = RunningTasks.find(childPid);
    if (stoppedProcess == RunningTasks.end()) {
        // Just ignore unknown childs - they may be spawned from
        // http server thread
        LOG("Skipped processing unknown stopped task with pid " << childPid);
        return;
    }

    TTargetsList::const_iterator stoppedTarget = Targets.find((stoppedProcess->second).Target);
    if (stoppedTarget == Targets.end()) {
        // Orphan task stopped; forget it?
        LOG("Skipped processing orphan stopped task with pid " << childPid);
        return;
    }

    TTargetTypeParameters::TId taskId = (*stoppedTarget)->Type->GetParameters().GetId(stoppedProcess->second.NTask);
    (*stoppedTarget)->Stopped(taskId, watcher);
}

void TWorkerGraph::ProcessContinuedTask(pid_t childPid, TGraphChangeWatcher &watcher) {
    TPidMap::iterator continuedProcess = RunningTasks.find(childPid);
    if (continuedProcess == RunningTasks.end()) {
        // Just ignore unknown childs - they may be spawned from
        // http server thread
        LOG("Skipped processing unknown continued task with pid " << childPid);
        return;
    }

    TTargetsList::const_iterator continuedTarget = Targets.find((continuedProcess->second).Target);
    if (continuedTarget == Targets.end()) {
        // Orphan task continued; forget it?
        LOG("Skipped processing orphan continued task with pid " << childPid);
        return;
    }

    TTargetTypeParameters::TId taskId = (*continuedTarget)->Type->GetParameters().GetId(continuedProcess->second.NTask);
    (*continuedTarget)->Continued(taskId, watcher);
}

void TWorkerGraph::ProcessResourcesEvents() {
    TGuard<TMutex> guard(Mutex);

    TResourceManager::TEvents events;
    GetResourceManager().PullEvents(events);

    for (TResourceManager::TEvents::const_iterator i = events.begin(); i != events.end(); ++i) {
        const TTargetsList::const_iterator target = Targets.find(i->Userdata.first);
        const size_t ntask = i->Userdata.second;

        if (target == Targets.end()) {
            DEBUGLOG1(target, i->Userdata.first << ": resource event for unknown target pulled");
            continue;
        }

        if (!(*target)->Type->IsOnThisWorker()) {
            DEBUGLOG1(target, (*target)->Name << ": resource event for foreign target pulled");
            continue;
        }

        if (ntask >= (*target)->GetTasks().Size()) {
            DEBUGLOG1(target, (*target)->Name << ": resources event with invalid task " << ntask << " pulled");
            continue;
        }

        (*target)->ProcessResourcesEvent(*i);
    }
}

TString ResStateToStringExtended(bool isUsingResources, EResourcesState resState) {
    if (!isUsingResources) {
        return "NOT_NEEDED";
    }

    if (resState == RS_DEFAULT) {
        return "NOT_HANDLED";
    } else {
        return ToString<EResourcesState>(resState);
    }
}

TTargetTypeParameters::TId ParseAndValidateTaskN(const TWorkerGraph::TTargetsList::const_iterator& target, const TString& taskNStr) {
    unsigned taskN = 0;
    try {
        taskN = FromString<unsigned>(taskNStr);
    } catch (const yexception& /*e*/) {
        ythrow yexception() << "can't parse task number: " << taskNStr;
    }
    if (taskN >= (*target)->GetTasks().Size())
        ythrow yexception() << "wrong task number: " << taskN;
    return (*target)->Type->GetParameters().GetId(taskN);
}

void TWorkerGraph::FormatResourcesHint(const TString& target, const TString& taskNStr, IOutputStream& out) const {
    TGuard<TMutex> guard(Mutex);

    const TTargetsList::const_iterator t = Targets.find(target);

    if (t == Targets.end())
        ythrow TNoResourcesHint() << "target " << target << " not found";

    if (!(*t)->Type->IsOnThisWorker())
        ythrow TNoResourcesHint() << "target " << target << " is not served by this worker";

    TTargetTypeParameters::TId taskId = ParseAndValidateTaskN(t, taskNStr);
    const TWorkerTaskStatus& task = (*t)->GetTasks().At(taskId);

    out << '{';
    out << "\"target\":\"" << (*t)->Name << "\"";
    out << ",\"worker\":\"" << GetWorkerNameCheckDefined() << "\"";
    out << ",\"task\":" << taskId.GetN();
    TString solver = (task.ResState == RS_CONNECTED || task.ResState == RS_CONNECTING) ? task.Resources.GetSolver().ToString() : TString("");
    out << ",\"solver\":\"" << solver << "\"";
    bool sovlerIsLocal = (task.ResState == RS_CONNECTED || task.ResState == RS_CONNECTING) ? task.Resources.IsLocalSolver() : false;
    out << ",\"solverIsLocal\":" << (sovlerIsLocal ? "true" : "false");
    out << ",\"state\":\"" << ResStateToStringExtended((*t)->IsUsingResources(), task.ResState) << "\"";
    out << ",\"message\":\"" << EncodeXMLString(task.ResMessage.data()) << "\"";

    out << ",\"resourceStr\":\"" << EncodeXMLString((*t)->GetResourcesString().data()) << "\"";

    out << ",\"resources\":{";
    for (TResourcesStruct::const_iterator i = task.Resources.begin(); i != task.Resources.end(); ++i) {
        if (i != task.Resources.begin())
            out << ',';
        out << "\"" << i->Key << "\":{\"value\":\"" << Sprintf("%1.2f", i->Val) << "\",\"name\":\"" << i->Name << "\"}";
    }
    out << "}";

    out << ",\"priority\":\"" << Sprintf("%1.2f", (*t)->GetPriority()) << "\"";

    out << '}';
}

TWorkerGraph::TResourcesEventsThread::TResourcesEventsThread(TWorkerGraph* graph)
    : TThread(ThreadProc, reinterpret_cast<void*>(this))
    , Terminate(false)
    , Graph(graph)
{
}

TWorkerGraph::TResourcesEventsThread::~TResourcesEventsThread() {
    Terminate = true;
    GetResourceManager().Signal();
}

void* TWorkerGraph::TResourcesEventsThread::ThreadProc(void* param) noexcept {
    TResourcesEventsThread* const self = reinterpret_cast<TResourcesEventsThread*>(param);

    SetCurrentThreadName("resources events");

    try {
        while (!self->Terminate) {
            GetResourceManager().WaitI();
            self->Graph->ProcessResourcesEvents();
        }
    } catch (...) {
        Y_FAIL("TEventsHandlerThread got error: %s", CurrentExceptionMessage().data());
    }

    return nullptr;
}

void TWorkerGraph::Command(ui32 flags, const TTaskState& state) {
    for (TTargetsList::const_iterator t = Targets.begin(); t != Targets.end(); ++t)
        Command(flags, (*t)->GetName(), state);

    LOG("Command (" << TCommandMessage::FormatText("", -1, flags, state) << ") OK");
}

void TWorkerGraph::Command(ui32 flags, const TString& target, const TTaskState& state) {
    TTargetsList::const_iterator t = Targets.find(target);
    if (t == Targets.end()) {
        LOG("Command (" << TCommandMessage::FormatText(target, -1, flags, state) << ") FAILED: unknown target");
        return;
    }

    NGatherSubgraph::TMask input(&(*t)->Type->GetParametersHyperspace());
    for (unsigned i = 0; i < input.Size(); ++i)
        input.At(i) = true;

    Command(flags, **t, input, state);

    LOG("Command (" << TCommandMessage::FormatText(target, -1, flags, state) << ") OK");
}

void TWorkerGraph::Command(ui32 flags, const TString& target, int nTask, const TTaskState& state) {
    TTargetsList::const_iterator t = Targets.find(target);
    if (t == Targets.end()) {
        LOG("Command (" << TCommandMessage::FormatText(target, nTask, flags, state) << ") FAILED: unknown target");
        return;
    }

    if (!(*t)->Type->IsOnThisWorker()) {
        LOG("Command (" << TCommandMessage::FormatText(target, nTask, flags, state) << ") FAILED: target " << (*t)->GetName() << " is not on this worker");
        return;
    }

    NGatherSubgraph::TMask input(&(*t)->Type->GetParametersHyperspace());
    input.At((*t)->GetHyperspaceTaskIndex(nTask)) = true;

    Command(flags, **t, input, state);

    LOG("Command (" << TCommandMessage::FormatText(target, nTask, flags, state) << ") OK");
}

void TWorkerGraph::Command2(ui32 flags, const TString& target) {
    TTargetsList::const_iterator t = Targets.find(target);
    if (t == Targets.end()) {
        LOG("Command2 (" << TCommandMessage2::FormatText(target, TCommandMessage2::TRepeatedTasks(), flags) << ") FAILED: unknown target");
        return;
    }

    NGatherSubgraph::TMask input(&(*t)->Type->GetParametersHyperspace());
    for (unsigned i = 0; i < input.Size(); ++i)
        input.At(i) = true;

    Command(flags, **t, input, TS_UNDEFINED);

    LOG("Command2 (" << TCommandMessage2::FormatText(target, TCommandMessage2::TRepeatedTasks(), flags) << ") OK");
}

void TWorkerGraph::Command2(ui32 flags, const TString& target, const TCommandMessage2::TRepeatedTasks& tasks) {
    TTargetsList::const_iterator t = Targets.find(target);
    if (t == Targets.end()) {
        LOG("Command2 (" << TCommandMessage2::FormatText(target, tasks, flags) << ") FAILED: unknown target");
        return;
    }

    NGatherSubgraph::TMask input(&(*t)->Type->GetParametersHyperspace());
    for (TCommandMessage2::TRepeatedTasks::const_iterator task = tasks.begin(); task != tasks.end(); ++task) {
        input.At(*task) = true;
    }

    Command(flags, **t, input, TS_UNDEFINED);

    LOG("Command2 (" << TCommandMessage2::FormatText(target, tasks, flags) << ") OK");
}

void TWorkerGraph::Command(ui32 flags, const TWorkerTarget& startTarget, const NGatherSubgraph::TMask& input, const TTaskState& state) {
    TGuard<TMutex> guard(Mutex);

    using namespace NGatherSubgraph;

    typedef TResult<TWorkerTarget> TResult;

    TResult subgraph;
    bool needSubgraph = (flags & (TCommandMessage::CF_RECURSIVE_UP | TCommandMessage::CF_RECURSIVE_DOWN));
    if (needSubgraph) {
        EMode mode = (flags & TCommandMessage::CF_RECURSIVE_UP) ?
                M_COMMAND_RECURSIVE_UP :
                M_COMMAND_RECURSIVE_DOWN;

        GatherSubgraph<TWorkerGraphTypes, TParamsByTypeHyperspace, TWorkerTargetEdgePredicate>
            (startTarget, input, mode, &subgraph);
    } else { // Lets construct fake NGatherSubgraph2::TResult with only one node - root target
        TResultForTarget* result = subgraph.InsertTargetResult(&startTarget, input);
        result->Skipped = TResultForTarget::SS_NOT_SKIPPED;

        TResult::TTargetList topoSortedTargets;
        topoSortedTargets.push_back(&startTarget);
        subgraph.SetTopoSortedTargets(topoSortedTargets);
    }

    Y_VERIFY(!subgraph.GetTopoSortedTargets().empty()); // Shouldn't be possible as
            // TopoSortedSubgraphTargetIsOk/TopoSortedSubgraphEdgeIsOk is always true

    TResult::TTargetList targetsToProcess = subgraph.GetTopoSortedTargets();

    if (flags & TCommandMessage::CF_RECURSIVE_ONLY) {
        targetsToProcess.pop_back(); // This excludes the root target
    }

    TGraphChangeWatcher watcher;

    for (TResult::TTargetList::const_iterator i = targetsToProcess.begin(); i != targetsToProcess.end(); ++i) {
        TWorkerTarget* target = const_cast<TWorkerTarget*>(*i);

        if (target->Type->IsOnThisWorker()) {
            TResult::TResultByTarget::const_iterator resultForTargetIterator = subgraph.GetResultByTarget().find(target);
            Y_VERIFY(resultForTargetIterator != subgraph.GetResultByTarget().end());
            TResultForTarget* result = resultForTargetIterator->second;

            const TMask& maskHyperspace = result->Mask;
            TMask mask = ProjectMaskOnLocalspace(*target, maskHyperspace);

            for (unsigned task = 0; task != mask.Size(); ++task) {
                if (state == TS_UNDEFINED || state == target->Tasks.At(task).GetState()) {
                    TTargetTypeParameters::TId taskId(&target->Type->GetParameters(), task);
                    if (mask.At(task) == true) {
                        if (result->IsNotSkipped())
                            target->CommandOnTask(taskId, flags, watcher);
                        else
                            target->CommandOnTask(taskId, flags | TCommandMessage::CF_MARK_SKIPPED, watcher);
                    }
                }
            }
        }
    }

    TryReadySomeTasks(watcher);
    watcher.Commit();
}

void TWorkerGraph::ExportFullStatus(TFullStatusMessage *message) const {
    TGuard<TMutex> guard(Mutex);

    LOG("Exporting full status");

    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        TFullStatusMessage::TTaskStatuses *messageTarget = message->AddTarget();

        for (TWorkerTarget::TTaskList::TConstEnumerator task = (*target)->GetTasks().Enumerator(); task.Next(); ) {
            TFullStatusMessage::TTaskStatus *status = messageTarget->AddStatus();

            propagateStatistics(status, task);
        }
    }
}

void TWorkerGraph::DumpStateExtra(TPrinter& out) const {
    Variables.DumpState(out);
}

void TWorkerGraph::CalculateTargetPriorities() {
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
         TWorkerTarget::TOptionsMap UserOptions = (*target)->GetOptions();
         for (TWorkerTarget::TOptionsMap::const_iterator option = UserOptions.begin();
                 option != UserOptions.end(); ++option) {
             if (option->first == "priority") {
                 float Priority = FromString<float>(option->second);
                 if (Priority < 0.0f)
                     Priority = 0.0f;
                 if (Priority > 1.0f)
                     Priority = 1.0f;
                 (*target)->SetPriority(Priority);
             } else if (option->first == "recpriorityup" || option->first == "recpriority") {
                 float Priority = FromString<float>(option->second);
                 DEBUGLOG("Recursive priority UP = " << Priority << " from target " << (*target)->Name);
                 if (Priority < 0.0f)
                     Priority = 0.0f;
                 if (Priority > 1.0f)
                     Priority = 1.0f;

                 (*target)->SetPriority(Priority);
                 TWorkerTarget::TTraversalGuard guard;
                 (*target)->BumpPriority(Priority, guard, TWorkerTarget::UP);
             } else if (option->first == "recprioritydown") {
                 float Priority = FromString<float>(option->second);
                 DEBUGLOG("Recursive priority DOWN = " << Priority << " from target " << (*target)->Name);
                 if (Priority < 0.0f)
                     Priority = 0.0f;
                 if (Priority > 1.0f)
                     Priority = 1.0f;

                 (*target)->SetPriority(Priority);
                 TWorkerTarget::TTraversalGuard guard;
                 (*target)->BumpPriority(Priority, guard, TWorkerTarget::DOWN);
             }
         }
     }

     /* Calculate initial target's priorities                   */
     /* Boost priority for target with big number of successors */
     /* We'll look only for two levels deep                     */
     size_t maxFollowersNum = 0;
     size_t subFollowersNum = 0;
     for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target)
         if ((*target)->Followers.size() > maxFollowersNum) {
             subFollowersNum = 0;
             for (TWorkerTarget::TDependsList::const_iterator follower = (*target)->Followers.begin();
                     follower != (*target)->Followers.end(); ++follower) {
                 subFollowersNum += follower->GetTarget()->Followers.size();
             }
             maxFollowersNum = (*target)->Followers.size();
         }

     for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
         if (!(*target)->GetPriority()) {
             float directFollowersNumber = 0;
             size_t secondFollowersNumber = 0;
             for (TWorkerTarget::TDependsList::const_iterator followers = (*target)->Followers.begin();
                     followers != (*target)->Followers.end(); ++followers) {
                 directFollowersNumber++;
                 secondFollowersNumber += followers->GetTarget()->Followers.size();
             }

             (*target)->SetPriority((directFollowersNumber + 0.5 * secondFollowersNumber) / (2 * (maxFollowersNum + 0.5 * subFollowersNum)));
         }
     }
}

void TWorkerGraph::ImportConfig(const TConfigMessage *message, bool dump) {
    TGuard<TMutex> guard(Mutex);

    LOG("Importing config");

    WorkerName = message->GetWorkerName();

    // XXX: it's bad to change external object - we should actually hold
    // ListManager inside the class. To fix after Graph detemplatization
    ListManager->ImportLists(message);

    ParseConfig(message);

    ApplyGlobalVariables();

    DEBUGLOG("Importing config OK");

    LOG("Defining resources");

    CalculateTargetPriorities();

    TGraphChangeWatcher watcher;
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target)
        if ((*target)->Type->IsOnThisWorker())
            (*target)->DefineResources(watcher);

    watcher.Commit(TGraphChangeWatcher::COMMIT_RESOURCES);

    if (dump) {
        TFsPath dumpTxt = TFsPath(WorkerGlobals->VarDirPath).Child("dump.txt");
        TFsPath dumpTxtTmp = dumpTxt.GetPath() + ".tmp";

        LOG("Dumping graph state to " << dumpTxt);

        {
            TFixedBufferFileOutput os(dumpTxtTmp);
            TPrinter printer(os);
            DumpState(printer);
            os.Flush();
        }

        dumpTxtTmp.RenameTo(dumpTxt);
    }

    CheckState();

    DEBUGLOG("Defining resources OK");
}

NProto::TVariablesDump TWorkerGraph::PackVariables() const {
    NProto::TVariablesDump result;
    for (const auto& item : Variables.GetMap()) {
        NProto::TVariablesDump::TVariable* dst = result.AddVariable();
        dst->SetName(item.Name);
        dst->SetValue(item.Value);
    }
    result.SetUpdateTimestamp(MilliSeconds());
    return result;
}

void TWorkerGraph::SaveVariablesToYt(const NProto::TVariablesDump &protoState) const {
    TMaybe<TYtStorage>& storage = WorkerGlobals->YtStorage;
    if (storage) {
        storage->SaveProtoStateToYt(TWorkerVariables::VariablesYtKey, protoState);
    }
}

void TWorkerGraph::SaveVariablesToDisk(const NProto::TVariablesDump &protoState) const {
    ::SaveProtoStateToDisk(GetVariablesFilePath(), protoState);
}

void TWorkerGraph::SaveVariables() const {
    LOG("Saving variables");
    const NProto::TVariablesDump protoState = PackVariables();
    SaveVariablesToDisk(protoState);
    SaveVariablesToYt(protoState);
    LOG("Saved variables");
}

void TWorkerGraph::ImportVariables(const TVariablesMessage *message) {
    TGuard<TMutex> guard(Mutex);

    LOG("Importing variables");

    Variables.IncLastRevision();
    Variables.Update(*message);

    TGraphChangeWatcher watcher;
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target)
        if ((*target)->Type->IsOnThisWorker() && (*target)->GetResourcesUseVariables())
            (*target)->DefineResources(watcher);
    watcher.Commit(TGraphChangeWatcher::COMMIT_RESOURCES);
    DEBUGLOG("Importing variables OK");

    SaveVariables();
}

void TWorkerGraph::ExportVariables(TVariablesMessage *message) const {
    TGuard<TMutex> guard(Mutex);

    LOG("Exporting variables");

    TVariablesMessage(Variables).Swap(message);

    DEBUGLOG("Exporting variables OK");
}

const TWorkerVariables& TWorkerGraph::GetVariables() const {
    return Variables;
}

void TWorkerGraph::ApplyGlobalVariables() {
    TGuard<TMutex> guard(Mutex);

    LOG("Applying global variables");

    for (TVariablesMap::const_iterator var = StrongVariables.begin(); var != StrongVariables.end(); ++var)
        Variables.ApplyStrongVariable(var->first, var->second);

    for (TVariablesMap::const_iterator var = DefaultVariables.begin(); var != DefaultVariables.end(); ++var)
        Variables.ApplyDefaultVariable(var->first, var->second);

    DEBUGLOG("Applying global variables OK");
}

TString LowSpaceCause(bool forced, i64 available, ui64 threshold) {
    TString result;
    TStringOutput out(result);
    if (forced)
        out << "forced";
    else
        out << available << ((available < (i64) threshold) ? "<" : ">=") << threshold;
    return result;
}

void TWorkerGraph::CheckLowFreeSpace() {
    TGuard<TMutex> guard(Mutex);

    if (Variables.GetMap().find("CM_INT_USE_STOP") != Variables.GetMap().end()) {
        TWorkerVariables::TMapType::const_iterator var;

        var = Variables.GetMap().find("CM_INT_STOP_THR");
        ui64 freeDiskSpaceLowThreshold;
        if (var != Variables.GetMap().end())
            freeDiskSpaceLowThreshold = FromString<ui64>(var->Value);
        else
            freeDiskSpaceLowThreshold = UINT64_C(5) * 1024 * 1024 * 1024;

        var = Variables.GetMap().find("CM_INT_CONT_THR");
        ui64 freeDiskSpaceHighThreshold;
        if (var != Variables.GetMap().end())
            freeDiskSpaceHighThreshold = FromString<ui64>(var->Value);
        else
            freeDiskSpaceHighThreshold = UINT64_C(10) * 1024 * 1024 * 1024;

        if (freeDiskSpaceLowThreshold >= freeDiskSpaceHighThreshold) {
            ERRORLOG("Low threshold " << freeDiskSpaceLowThreshold << " should be less than high threshold " << freeDiskSpaceHighThreshold);
            return;
        }

        if (freeDiskSpaceLowThreshold < UINT64_C(1) * 1024 * 1024 * 1024) {
            ERRORLOG("Low threshold is " << freeDiskSpaceLowThreshold << ", but should be more than or equal to " << UINT64_C(1) * 1024 * 1024 * 1024);
            return;
        }

        if (freeDiskSpaceHighThreshold - freeDiskSpaceLowThreshold < UINT64_C(1) * 1024 * 1024 * 1024) {
            ERRORLOG("The difference between high threshold and low threshold is " << freeDiskSpaceHighThreshold - freeDiskSpaceLowThreshold << ", but should be more than " << UINT64_C(1) * 1024 * 1024 * 1024);
            return;
        }

        TMaybe<bool> forceLowFreespace; // undefined - not forced, true - forced low space, false - forced enough space
        var = Variables.GetMap().find("CM_INT_FORCE_LOW_FREESPACE");
        if (var != Variables.GetMap().end()) {
            TString forceLowFreespaceStr = var->Value;
            if (forceLowFreespaceStr == "yes")
                forceLowFreespace = true;
            else
                forceLowFreespace = false;
        }

        TDiskspaceMessage msg(WorkerGlobals->VarDirPath.data());
        if (!LowFreeSpaceModeOn && (forceLowFreespace.Defined() ? *forceLowFreespace : (msg.GetAvail() < (i64) freeDiskSpaceLowThreshold))) {
            ERRORLOG("Low free space (" << LowSpaceCause(forceLowFreespace.Defined(), msg.GetAvail(), freeDiskSpaceLowThreshold) << "), stopping tasks");
            StopTasks();
            LowFreeSpaceModeOn = true;
        } else if (LowFreeSpaceModeOn && (forceLowFreespace.Defined() ? !*forceLowFreespace : (msg.GetAvail() >= (i64) freeDiskSpaceHighThreshold))) {
            LOG("Enough free space (" << LowSpaceCause(forceLowFreespace.Defined(), msg.GetAvail(), freeDiskSpaceHighThreshold) << "), continuing tasks");
            ContinueTasks();
            LowFreeSpaceModeOn = false;
        }
    }
}

void TWorkerGraph::CheckExpiredTasks() {
    TGuard<TMutex> guard(Mutex);

    TGraphChangeWatcher watcher;
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target)
        (*target)->CheckExpiredTasks(watcher);

    watcher.Commit();
}

void TWorkerGraph::StopTasks() {
    TGuard<TMutex> guard(Mutex);

    TGraphChangeWatcher watcher;
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        if (!(*target)->GetDontStop())
            (*target)->StopTasks(watcher);
    }

    watcher.Commit();
}

void TWorkerGraph::ContinueTasks() {
    TGuard<TMutex> guard(Mutex);

    TGraphChangeWatcher watcher;
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target)
        (*target)->ContinueTasks(watcher);

    watcher.Commit();
}

void TWorkerGraph::ProcessLostTasks(bool restartLostTasks) {
    TGuard<TMutex> guard(Mutex);

    if (LostTargets.empty()) {
        return;
    }
    TLostTargetsMMap finished;
    CheckLostTasks(&finished);
    if (finished.empty()) {
        return;
    }

    if (!restartLostTasks) {
        ProcessFinishedLostTasks(finished);
    } else {
        RestartLostTasks(finished);
    }
}

void TWorkerGraph::CheckLostTasks(TLostTargetsMMap *finished) {
    TLostTargetsMMap newLostTargets;
    for (const auto& [name, pair] : LostTargets) {
        DEBUGLOG("Checking lost target " << name);
        const auto& [target, num] = pair;
        const TWorkerTaskStatus& task = target->GetTasks().At(num);
        pid_t pid = task.GetPid();
        bool taskIsRunning = target->CheckTaskAvailability(pid, task.GetLastStarted(), task.GetLastProcStarted());
        if (taskIsRunning) {
            DEBUGLOG("Lost target " << name << " is still running");
            newLostTargets.insert({name, pair});
        } else {
            LOG("Lost target " << target->Name << " task " << num << " has finished");
            finished->insert({name, pair});
        }
    }
    LostTargets.swap(newLostTargets);
}

void TWorkerGraph::ProcessFinishedLostTasks(const TLostTargetsMMap &finished) {
    if (finished.empty()) {
        return;
    }
    LOG("Finishing " << finished.size() << " lost tasks");
    TGraphChangeWatcher watcher;
    for (const auto& [name, pair] : finished) {
        LOG("Finishing lost target " << name);
        const auto& [target, num] = pair;
        const TWorkerTaskStatus& task = target->GetTasks().At(num);
        pid_t pid = task.GetPid();
        ProcessFinishedTask(pid, watcher);
    }
    watcher.Commit();
}

void TWorkerGraph::RestartLostTasks(const TLostTargetsMMap &finished) {
    LOG("Restarting " << finished.size() << " lost tasks");
    TGraphChangeWatcher watcher;
    TLostTargetsMMap notRestarted;
    for (const auto&[name, pair] : finished) {
        const auto&[target, num] = pair;
        if (target->GetNoRestartAfterLoss()) {
            LOG("Target " << name << " is marked with no_restart_after_loss, skipping it");
            notRestarted.insert({name, pair});
            continue;
        }

        /* lost start cleanup */
        auto lostPid = target->Tasks[num].GetPid();
        RunningTasks.erase(lostPid);
        if (!target->Tasks[num].DoNotTrack()) {
            target->Type->Graph->ResourceMonitor.rmProcessGroup(lostPid);
        }

        /* restart */
        TTargetTypeParameters::TId tId = target->Type->GetParameters().GetId(num);
        target->Tasks[num].SetPid(-1);
        target->Tasks[num].SetLostState(false);
        target->ChangeState(watcher, tId, TS_RUNNING);
    }
    watcher.Commit();
    if (!notRestarted.empty()) {
        ProcessFinishedLostTasks(notRestarted);
    }
}

void TWorkerGraph::SetAllTaskState(const TTaskState& state) {
    for (TTargetsList::const_iterator target = Targets.begin(); target != Targets.end(); ++target) {
        (*target)->SetAllTaskState(state);
    }
}

TAutoPtr<TNetworkAddress> TWorkerGraph::GetSolverHttpAddressForTask(const TString& targetName, const TString& taskNStr) {
    TGuard<TMutex> guard(Mutex);

    TWorkerGraph::TTargetsList::const_iterator target = Targets.find(targetName);

    if (target == Targets.end())
        ythrow yexception() << "target not found: " << targetName;
    if (!(*target)->Type->IsOnThisWorker())
        ythrow yexception() << "target is not on this worker: " << targetName;

    TTargetTypeParameters::TId taskId = ParseAndValidateTaskN(target, taskNStr);
    const TWorkerTaskStatus& task = (*target)->GetTasks().At(taskId);

    const ISockAddr& solver = task.Resources.GetSolver();

    if (const TSockAddrInet* const solverInet = dynamic_cast<const TSockAddrInet*>(&solver)) {
        return TAutoPtr<TNetworkAddress>(new TNetworkAddress(IpToString(solverInet->GetIp()), WorkerGlobals->SolverHttpPort));
    } else if (const TSockAddrInet6* const solverInet6 = dynamic_cast<const TSockAddrInet6*>(&solver)) {
        return TAutoPtr<TNetworkAddress>(new TNetworkAddress(solverInet6->GetIp(), WorkerGlobals->SolverHttpPort));
    } else {
        ythrow yexception() << "task is connected to solver by local socket";
    }
}

