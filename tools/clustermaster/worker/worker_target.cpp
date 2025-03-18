#include <tools/clustermaster/common/util.h>
#include <tools/clustermaster/common/vector_to_string.h>

#include <library/cpp/deprecated/split/split_iterator.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/datetime/base.h>
#include <util/digest/sequence.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/util.h>
#include <util/system/compat.h>
#include <util/system/dynlib.h>
#include <util/system/error.h>
#include <util/system/fs.h>
#include <util/system/hostname.h>
#include <util/system/sigset.h>

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#ifndef _win_
#   include <sys/wait.h>
#endif

#ifdef __linux__
#   include <sys/time.h>
#endif

#include "worker_target.h"

#include "graph_change_watcher.h"
#include "log.h"
#include "master_multiplexor.h"
#include "messages.h"
#include "state_file.h"
#include "worker.h"
#include "worker_target_graph.h"
#include "worker_variables.h"

extern char** environ;

TWorkerTarget::TWorkerTarget(const TString& n, TWorkerTargetType *t, const TParserState& parserState)
    : TTargetBase<TWorkerGraphTypes>(n, t, parserState)
      , DoUseAnyResource(true)
      , ResourcesUseVariables(true)
      , Timeout(0) // no timeout by default
      , Priority(0.0f) // lowest priority by default
      , jumpTimes(0)
      , DontStop(false)
      , NoResetOnTypeChange(false)
      , NoRestartAfterLoss(false)
{
    if (!Type->IsOnThisWorker()) {
        return;
    }

    const ::NProto::TConfigMessage_TThisWorkerTarget* msgTarget = nullptr;
    for (int i = 0; i < parserState.Message->GetThisWorkerTargets().size(); ++i) {
        const ::NProto::TConfigMessage_TThisWorkerTarget& msgTarget0 = parserState.Message->GetThisWorkerTargets(i);
        if (msgTarget0.GetName() != Name) {
            continue;
        }

        msgTarget = &msgTarget0;
    }

    if (!msgTarget) {
        ythrow TWithBackTrace<yexception>() << "missing target node for target " << n;
    }

    HasCrossnodeDepends = msgTarget->GetHasCrossnodeDepends();

    for (TTaskList::TEnumerator task = Tasks.Enumerator(); task.Next(); )
        task->Resources.SetTargetName(Name);

    for (THashMap<TString, TString>::iterator i = t->Options.begin(); i != t->Options.end(); ++i)
        AddOption(i->first, i->second);
}

TWorkerTarget::~TWorkerTarget() {
    UndefResources();
}

void TWorkerTarget::SetPriority(float prio) {
    if (prio < Type->Graph->WorkerGlobals->PriorityRange.second) {
        Priority = prio;
    } else {
        Priority = Type->Graph->WorkerGlobals->PriorityRange.second;
    }
}

void TWorkerTarget::SetAllTaskState(const TTaskState& state) {
    for (TWorkerTarget::TTaskIterator task = TaskIterator(); task.Next(); ) {
        task->SetState(state);
    }
}

void TWorkerTarget::GetFollowers(TSet<TString> const& notFollowThrew, TSet<TString>* allFollowers)
{
    TList<TWorkerTarget *> travers;
    travers.push_back(this);

    while (!travers.empty()) {
        TWorkerTarget *currentTarget = travers.front();
        if (notFollowThrew.find(currentTarget->Name) == notFollowThrew.end()) {
            for (TWorkerTarget::TDependsList::iterator follower = currentTarget->Followers.begin();
                                follower != currentTarget->Followers.end(); ++follower) {
                travers.push_back(follower->GetTarget());
            }
        }

        allFollowers->insert(currentTarget->Name);
        travers.pop_front();
    }

    return;
}

void TWorkerTarget::GetDependers(TSet<TString> const& notFollowThrew, TSet<TString>* allDependers)
{
    TList<TWorkerTarget *> travers;
    travers.push_back(this);

    while (!travers.empty()) {
        TWorkerTarget *currentTarget = travers.front();
        if (notFollowThrew.find(currentTarget->Name) == notFollowThrew.end()) {
            for (TWorkerTarget::TDependsList::iterator depender = currentTarget->Depends.begin();
                                depender != currentTarget->Depends.end(); ++depender) {
                travers.push_back(depender->GetTarget());
            }
        }

        allDependers->insert(currentTarget->Name);
        travers.pop_front();
    }

    return;
}

void TWorkerTarget::AddOption(const TString &key, const TString &value) {
    UserOptions[key] = value;

    /*
     * Priorities will be handled in whole graph post-processing procedure
     */
    if (key == "res") {
        if (value.size())
            ResourcesString = value;
        else
            DoUseAnyResource = false;
     } else if (key == "semlimit") {
        if (Semaphore.Get() == nullptr)
            Semaphore = new TSemaphore(this);

        Semaphore->SetLimit(atoi(value.data()));
    } else if (key == "timeout") {
        if (value == "unlimited")
            Timeout = 0;
        else
            Timeout = atoi(value.data());
    } else if (key == "jumpto") {
        TString jumpToTargetName;

        TSplitDelimiters delims(":");
        TDelimitersStrictSplit split(value.data(), value.size(), delims);
        TDelimitersStrictSplit::TIterator it = split.Iterator();

        jumpToTargetName = it.NextString();
        TWorkerGraph::const_iterator jumpToTargetit = Type->Graph->find(jumpToTargetName);
        if (jumpToTargetit == Type->Graph->end())
            return;

        jumpCondition = it.NextString();
        DEBUGLOG(Name << " jumps to " << jumpToTargetName << " if " << jumpCondition);

        size_t pos = jumpCondition.find('<');
        if (jumpCondition.find("inf", pos) != TString::npos) {
            jumpTimes = JUMP_UNLIMITED;
        } else try {
            jumpTimes = FromString<int>(TStringBuf(jumpCondition, pos + 1 , TString::npos));
        } catch (const yexception&) {
            ythrow yexception() << "Wrong syntax for jump jumpCondition:" << Name << " -> " << jumpToTargetName << " if " << jumpCondition;
        }

        TSet<TString> allFollowers, allDependers, skipList;
        skipList.insert(Name);
        (*jumpToTargetit)->GetFollowers(skipList, &allFollowers);
        skipList.clear();
        skipList.insert(jumpToTargetName);
        GetDependers(skipList, &allDependers);

        DEBUGLOG(Name << " jumps to [" << allFollowers.size() << "/" << allDependers.size() << "]");

        for (TSet<TString>::iterator it = allDependers.begin();
                it != allDependers.end(); ++it) {
            if (allFollowers.find(*it) != allFollowers.end()) {
                conditionalJumpTargets.push_back(*it);
                DEBUGLOG("jumps to RESTART: " << (*it) << " " << conditionalJumpTargets.size());
            } else {
                DEBUGLOG("jumps to there is no: " << (*it));
            }
        }
    } else if (key == "dont_stop") {
        DontStop = true;
    } else if (key == "no_reset_on_type_change") {
        NoResetOnTypeChange = true;
    } else if (key == "no_restart_after_loss") {
        NoRestartAfterLoss = FromString<bool>(value);
    } else if (key == "mailto" || key == "smsto" || key == "graphtag" || key == "restart_on_success" || key == "retry_on_failure") {
        /* these are used only on the master side */
    } else {
        ERRORLOG(Name << " : unknown option: " << key << "=" << value);
    }
}

bool TWorkerTarget::IsSame(const TWorkerTarget* right) const {
    return Name == right->Name && Tasks.Size() == right->Tasks.Size();
}

void TWorkerTarget::SwapState(TWorkerTarget* right) {
    Y_VERIFY(IsSame(right), "must be the same target");
    DoSwap(Tasks, right->Tasks);
    CheckState();
}

void TWorkerTarget::LoadState() {
    if (Tasks.Empty())
        return; // worker doesn't process this target

    auto loadStateForTask = [&] (const NProto::TTaskStatus& status, TTaskList::TEnumerator& task) {
        propagateStatistics(task, &status);
        const TTaskState& state = TTaskState::Make(status.GetState(), status.GetStateValue());
        task->SetState(state != TS_RUNNING ? state : TS_READY);
        task->SetRepeatMax(jumpTimes);

        for (TTaskStatuses::TRepeatedSharedRes::const_iterator shared = status.GetSharedRes().begin();
                shared != status.GetSharedRes().end(); ++shared) {
            task->AddSharedResDefinition(shared->GetName(), shared->GetVal());
        }

        pid_t pid = task->GetPid();
        if (pid > 0) {  //task was run, but was lost
            task->SetLostState(true);
            Type->Graph->AddLostTask(this, task.CurrentN().GetN());
            TGraphChangeWatcher watcherStub; // is used to fulfil TryToRun signature only (Commit() isn't called)
            Type->Graph->TaskStarted(this, task.CurrentN().GetN(), TryToRun(task.CurrentN(), watcherStub)); // Fake run, to initialize target like running
        } else {
            task->SetLostState(false);
        }
    };

    TMaybe<TTaskStatuses> savedStatuses = LoadTargetStatuses();
    if (!savedStatuses) {
        return;
    }
    if (NoResetOnTypeChange) {
        THashMap<TTargetTypeParameters::TPath, NProto::TTaskStatus, TRangeHash<>> savedStatusesMap;
        for (auto& status : *savedStatuses->MutableStatus()) {
            TTargetTypeParameters::TPath path;
            path.reserve(status.PathSize());
            for (const auto& pathPart : status.GetPath()) {
                path.push_back(pathPart);
            }
            savedStatusesMap[path] = status;
        }
        for (TTaskList::TEnumerator task = Tasks.Enumerator(); task.Next();) {
            if (savedStatusesMap.contains(task.CurrentPath())) {
                loadStateForTask(savedStatusesMap.at(task.CurrentPath()), task);
            }
        }

    } else {
        TTaskStatuses::TRepeatedStatus::const_iterator status = savedStatuses->GetStatus().begin();
        for (TTaskList::TEnumerator task = Tasks.Enumerator(); task.Next(); ++status) {
            loadStateForTask(*status, task);
        }

    }
}

TMaybe<TTaskStatuses> TWorkerTarget::LoadTargetStatusesFromYt() const {
    TMaybe<TYtStorage>& storage = Type->Graph->WorkerGlobals->YtStorage;
    if (!storage) {
        return {};
    }
    return storage->LoadAndParseStateFromYt<TTaskStatuses>(GetName());
}

TMaybe<TTaskStatuses> TWorkerTarget::LoadTargetStatusesFromDisk() const {
    return ::LoadAndParseStateFromDisk<TTaskStatuses>(GetStateFilePath());
}

TMaybe<TTaskStatuses> TWorkerTarget::LoadTargetStatuses() const {
    TVector<TMaybe<TTaskStatuses>> candidates;
    candidates.emplace_back(LoadTargetStatusesFromYt());
    candidates.emplace_back(LoadTargetStatusesFromDisk());

    TMaybe<TTaskStatuses> newest = ::SelectByUpdateTimestamp(candidates);
    if (newest.Empty()) {
        ERRORLOG("Failed to load state for target " << GetName() << " from any source");
        return {};
    }

    // When NoResetOnTypeChange is false, if number of tasks had changed, forget state
    if (!NoResetOnTypeChange && (newest->StatusSize() != Tasks.Size())) {
        ERRORLOG("Task count mismatch");
        return {};
    }

    return newest;
}

size_t TWorkerTarget::GetHyperspaceTaskIndex(size_t localspaceTaskIndex) const {
    Y_VERIFY(Type->GetLocalspaceShift().Defined());
    return localspaceTaskIndex + Type->GetLocalspaceShift()->GetShift();
}

TTaskStatuses TWorkerTarget::SerializeStateToProtobuf() const {
    TTaskStatuses state;
    state.SetUpdateTimestamp(MilliSeconds());

    for (TTaskList::TConstEnumerator task = Tasks.Enumerator(); task.Next(); ) {
        NProto::TTaskStatus* status = state.AddStatus();
        propagateStatistics(status, task);
        if (!!task->GetResStat().sharedAreas) {
            for (TMap<TString, double>::iterator it = task->GetResStat().sharedAreas->begin();
                it != task->GetResStat().sharedAreas->end(); ++it) {
                NProto::TTaskStatus::TSharedDefinition *shared = status->AddSharedRes();
                shared->SetName(it->first);
                shared->SetVal(it->second);
            }
        }
        for (const auto& pathPart : task.CurrentPath()) {
            status->AddPath(pathPart);
        }
    }

    return state;
}

const TFsPath TWorkerTarget::GetStateFilePath() const {
    return StateFilePath(Type->Graph->WorkerGlobals->VarDirPath, GetName());
}

void TWorkerTarget::SaveStateToYt(const TTaskStatuses& protoState) const {
    TMaybe<TYtStorage>& storage = Type->Graph->WorkerGlobals->YtStorage;
    if (storage) {
        storage->SaveProtoStateToYt(GetName(), protoState);
    }
}

void TWorkerTarget::SaveStateToDisk(const TTaskStatuses& protoState) const {
    ::SaveProtoStateToDisk(GetStateFilePath(), protoState);
}

void TWorkerTarget::SaveState() const {
    if (Type->Graph->ForTest) {
        return;
    }
    TTaskStatuses protoState = SerializeStateToProtobuf();
    SaveStateToDisk(protoState);
    SaveStateToYt(protoState);
}

void TWorkerTarget::ChangeState(TGraphChangeWatcher& watcher, TTargetTypeParameters::TId nTask, const TTaskState& newState, bool needsSingleStatus) {
    TWorkerTaskStatus* task = &Tasks.At(nTask);
    watcher.Register(this, nTask, task->GetState(), newState, needsSingleStatus);
    task->SetState(newState);
    task->SetLastChanged(time(nullptr));
}

void TWorkerTarget::ChangeStateThroughRunning(TGraphChangeWatcher& watcher, TTargetTypeParameters::TId nTask, const TTaskState& newState, bool needsSingleStatus) {
    const TTaskState& st = Tasks[nTask].GetState();

    // Changing task state to TS_RUNNING if the task has immediately finished after waking up in order to release the claimed resources
    if (st == TS_CONTINUING || st == TS_STOPPED)
        ChangeState(watcher, nTask, TS_RUNNING, needsSingleStatus);

    ChangeState(watcher, nTask, newState, needsSingleStatus);
}

const TTaskState& TWorkerTarget::ReadyToGoState() const {
    return IsUsingResources() ? TS_READY : TS_RUNNING;
}

const TTaskState& TWorkerTarget::ReadyToGoStateOrSuspended() const {
    if (Type->Graph->IsLowFreeSpaceModeOn() && !GetDontStop()) {
        return TS_SUSPENDED;
    } else {
        return ReadyToGoState();
    }
}

void TWorkerTarget::PokeReady(const TMaybe<TTargetTypeParameters::TId>& nTask, TGraphChangeWatcher& watcher) {
    if (!nTask) {
        PokeReadyAllTasks(watcher);
    } else {
        const TTaskState& newState = ReadyToGoStateOrSuspended();
        if (Tasks[*nTask].GetState() == TS_PENDING)
            ChangeState(watcher, *nTask, newState);
    }
}

void TWorkerTarget::PokeReadyAllTasks(TGraphChangeWatcher& watcher) {
    const TTaskState& newState = ReadyToGoStateOrSuspended();
    for (TTaskList::TEnumerator task = Tasks.Enumerator(); task.Next(); ) {
        if (task->GetState() == TS_PENDING)
            ChangeState(watcher, task.CurrentN(), newState);
    }
}

time_t TWorkerTarget::GetProcStartTime(pid_t pid) {
#ifdef __FreeBSD__
    TString statusFile = Sprintf("/proc/%d/status", pid);
#elif defined __linux__
    TString statusFile = Sprintf("/proc/%d/stat", pid);
#else
    TString statusFile;
    ythrow yexception() << "GetProcStartTime(): doesn't yet realized";
#endif
    TString statusStr;
    try {
        TFileInput in(statusFile);
        statusStr = in.ReadLine();
    } catch (const yexception& e) {
        //there is no process with such pid, process died
        return (time_t)-1;
    }

    time_t processStartTime = (time_t)-1;
#ifdef __FreeBSD__
    TVector<TString> values = SplitString(statusStr, " ");
    Y_ASSERT(values.size() > 8);

    char startTimeStr[1024];
    strcpy(startTimeStr, ~values[7]);
    char *c = strchr(startTimeStr, ',');    //the process start time in seconds and microseconds, comma separated,
                                            //we need only seconds
    if (c) {
        *c = 0;
    } else {
        ERRORLOG("Invalid status string: " << statusStr << ", field " << values[7]);
        ythrow yexception() << "Invalid status string: " << statusStr << ", field " << values[7];
    }

    try {
        processStartTime = FromString<time_t>(startTimeStr);
    } catch (const yexception &e) {
        ERRORLOG("Can't parse status string: " << statusStr << ", field " << values[7]);
        throw;
    }
#elif defined __linux__
    TStringBuf buf{statusStr};
    TStringBuf afterNameStr = buf.RAfter(')');  // comm (field #2) can contain space or parentheses
    TVector<TStringBuf> values = StringSplitter(afterNameStr).Split(' ');

    Y_ASSERT(values.size() > 21); // excluding pid (field #1)
    ui64 lprcStartTimeJiff = FromString<ui64>(values[20]);  // starttime (field #22)

#ifdef HZ
    processStartTime = lprcStartTimeJiff / HZ;
#else
    processStartTime = lprcStartTimeJiff / 100;    //mostly used constant on linux
#endif

#else
    ythrow yexception() << "GetProcStartTime(): doesn't yet realized";
#endif
    return processStartTime;
}

time_t TWorkerTarget::GetStartTime(pid_t pid) {
    time_t procStartTime = GetProcStartTime(pid);
    if (procStartTime == (time_t)-1)
        return procStartTime;

    time_t processStartTime = (time_t)-1;
#ifdef __FreeBSD__
    processStartTime = procStartTime;
#elif defined __linux__
    TString uptimeStr;
    try {
        TUnbufferedFileInput in("/proc/uptime");
        uptimeStr = in.ReadLine();
    } catch (const yexception& e) {
        ythrow yexception() << "cannot read status file from procfs for process " << pid;
    }
    double up, idle;
    if (sscanf(uptimeStr.data(), "%lf %lf", &up, &idle) < 2) {
        ythrow yexception() << "cannot read values from /proc/uptime";
    }

    struct timeval tv;
    if (gettimeofday(&tv, nullptr)) {
        ythrow yexception() << "failed gettimeofday(): " << LastSystemErrorText();
    }
    double currentTime = tv.tv_sec + tv.tv_usec/1000000.0;
    processStartTime = (double)(currentTime - up) + procStartTime;
#else
    ythrow yexception() << "GetStartTime(): doesn't yet realized";
#endif

    return processStartTime;
}

bool TWorkerTarget::CheckTaskAvailability(pid_t pid, ui32 startedTime, ui32 procStartedTime) {
    time_t pidStartTime = (time_t)-1; // will be get from procfs
    time_t expectedStartTime = 0;

    // You should find started task process by pid and start time.
    // For some rare reasons GetStartTime may be nondeterministic in contrast to GetProcStartTime.
    // So try to find started task process by GetProcStartTime first.

    if (procStartedTime) {
        expectedStartTime = procStartedTime;
        pidStartTime = GetProcStartTime(pid);
    } else {  // process crashed during start or old version task state
        expectedStartTime = startedTime;
        pidStartTime = GetStartTime(pid);
    }

    if ((time_t)-1 == pidStartTime) {
        // already finished
        return false;
    }

    return (pidStartTime == expectedStartTime);
}

bool TWorkerTarget::GetDNTStatus() const {
    try {
        TFile DoNotTrackFileFd(Type->Graph->WorkerGlobals->DoNotTrackFlagPath, OpenExisting);
        if (DoNotTrackFileFd.IsOpen()) {
            return true;
        } else {
            return false;
        }
    } catch (const yexception &e) {
        return false;
    }
}

pid_t TWorkerTarget::TryToRun(const TTargetTypeParameters::TId& nTask, TGraphChangeWatcher& watcher) {
    DEBUGLOG("Try to RUN: Target " << Name << " Task" << nTask.GetN());
    try {
        pid_t pid = -1;
        if (Tasks[nTask].GetLostState()) {
            DEBUGLOG("Task " << nTask.GetN() << " for target " << Name << " already was run");
            pid = Tasks[nTask].GetPid();
        } else {
            pid = Run(nTask);
            time_t processProcStartTime = GetProcStartTime(pid);
            time_t processStartTime = GetStartTime(pid);
            Tasks[nTask].ToggleDoNotTtackMarker(GetDNTStatus());
            if (!Tasks[nTask].DoNotTrack()) {
                if ((time_t)-1 == processProcStartTime) { /* process died */
                    Tasks[nTask].SetLastStarted(time(nullptr));
                } else {
                    Tasks[nTask].SetLastProcStarted(processProcStartTime);
                    Tasks[nTask].SetLastStarted(processStartTime);
                }
            }
        }

        Tasks[nTask].SetPid(pid);
        Tasks[nTask].IncStartedCounter();

        if (!Tasks[nTask].DoNotTrack())
            Type->Graph->ResourceMonitor.startGroupMonitoring(pid);

        ChangeState(watcher, nTask, TS_RUNNING, true);
        Tasks[nTask].IncStartedCounter();

        return pid;
    } catch (const yexception &e) {
        Tasks[nTask].SetLastFailure(time(nullptr));
        Tasks[nTask].SetPid(-1);

        ChangeState(watcher, nTask, TS_FAILED, true);

        throw;
    }
}

pid_t TWorkerTarget::Run(const TTargetTypeParameters::TId& nTask) {
    DEBUGLOG("START: Target " << Name << " Task " << nTask.GetN() << " launching");

    // open log
    const TString logfilename = Sprintf("%s/%s.%03d.log", Type->Graph->WorkerGlobals->LogDirPath.data(), Name.data(), int(nTask.GetN()));
    struct stat buffer;

    if (Type->Graph->WorkerGlobals->NTargetLogs > 0) {
        if (stat(logfilename.data(), &buffer) == 0) {
            int lognum = Type->Graph->WorkerGlobals->NTargetLogs;
            TString nextlogfilename;
            TString prevlogfilename;
            while ((lognum--) >= 0) {
                if (lognum > 0) {
                    prevlogfilename = Sprintf("%s.%d", logfilename.data(), lognum);
                } else {
                    prevlogfilename = logfilename;
                }
                if (stat(prevlogfilename.data(), &buffer) == 0) {
                    nextlogfilename = Sprintf("%s.%d", logfilename.data(), lognum + 1);
                    rename(prevlogfilename.data(), nextlogfilename.data());
                }
            }
        }
    } else {
        unlink(logfilename.data());
    }

    TFileHandle logFile(logfilename, CreateAlways | WrOnly | ARW);

    if (!logFile.IsOpen())
        ythrow TSystemError() << "failed to open log file";

    // We need to do everything that require allocation before fork. Allocation could cause (and I observed real
    // cases) hang of forked process as allocator lock was acquired by thread that wasn't cloned by fork.

    const TString& shebang = Type->Graph->GetShebang();
    const size_t shellArgPos = shebang.find_first_of(" \t");  // shebang may take up to one argument
    const TString& shell = shebang.substr(0, shellArgPos);
    const size_t subshellPos = shebang.find_first_not_of(" \t", shellArgPos);
    const TString& subshell = subshellPos == TString::npos ? "" : shebang.substr(subshellPos);

    TVector<TString> argv_strings;
    TVector<TString> envp_strings;
    TVector<SOCKET> commSockets;

    TVector<char*> argv;
    TVector<char*> envp;

    TString devNullStr = TString("/dev/null");

    do {
        const bool isenv  = shell.EndsWith("/env");
        const bool isbash = shell.EndsWith("bash") || (isenv && subshell.EndsWith("bash"));
        const bool issh   = shell.EndsWith("/sh");

        argv_strings.push_back(Type->Graph->WorkerGlobals->BinaryPath);
        argv_strings.push_back("--mod-state");
        argv_strings.push_back("-v");
        argv_strings.push_back(Type->Graph->WorkerGlobals->VarDirPath);
        argv_strings.push_back("--target");
        argv_strings.push_back(Name);
        argv_strings.push_back("--task");
        argv_strings.push_back(ToString<int>(nTask.GetN()));
        argv_strings.push_back("--count");
        argv_strings.push_back(ToString<int>(Tasks.Size()));
        argv_strings.push_back("--");

        argv_strings.push_back(shell);
        if (isenv && !subshell.empty()) {
            argv_strings.push_back(subshell);
        }
        if (issh || isbash) {
            argv_strings.push_back("-e");
            argv_strings.push_back("-u");
        }
        if (isbash) {
            argv_strings.push_back("-o");
            argv_strings.push_back("pipefail");
        }

        argv_strings.push_back(Type->Graph->WorkerGlobals->ScriptPath);
        argv_strings.push_back(Name);

        TVector<TString> args = Type->GetParameters().GetScriptArgsOnWorkerByN(nTask);
        argv_strings.insert(argv_strings.end(), args.begin(), args.end());

        for (char** i = environ; *i; ++i) {
            if (Type->Graph->GetVariables().GetMap().find(TString(*i, TString(*i).find_first_of("="))) == Type->Graph->GetVariables().GetMap().end())
                envp_strings.push_back(*i);
        }

        envp_strings.push_back("CM_TO_REPEAT=" + ToString<int>(Tasks[nTask].GetToRepeat()));

        for (TWorkerVariables::TMapType::const_iterator i = Type->Graph->GetVariables().GetMap().begin(); i != Type->Graph->GetVariables().GetMap().end(); ++i) {
            envp_strings.push_back(i->Name + "=" + i->Value);
        }

        const TString MasterHttpAddr = Singleton<TMasterMultiplexor>()->GetMasterHttpAddr();
        if (!MasterHttpAddr.empty())
            envp_strings.push_back(TString("CMAPI=") + Type->Graph->WorkerGlobals->BinaryPath + " --cmremote " + MasterHttpAddr);

        // variable only for RobotRegressionTests
        envp_strings.push_back(TString("CM_TASK_NUMBER_FOR_TEST=") + ToString(nTask.GetN()));

        for (TVector<TString>::const_iterator i = argv_strings.begin(); i != argv_strings.end(); ++i)
            argv.push_back(const_cast<char*>(i->data()));

        argv.push_back(nullptr);

        for (TVector<TString>::const_iterator i = envp_strings.begin(); i != envp_strings.end(); ++i)
            envp.push_back(const_cast<char*>(i->data()));
        envp.push_back(nullptr);

        TAutoPtr< TVector<SOCKET> > sockets = GetResourceManager().GetSockets();
        commSockets.resize(sockets.Get()->size(), 0);
        for (size_t i = 0; i < sockets.Get()->size(); ++i) {
            commSockets[i] = (*sockets.Get())[i];
        }
    } while (false);

    // Do not do any allocation after fork()! See comment above.

#ifndef _win_

    pid_t childPid = fork();

    if (childPid == -1)
        ythrow TSystemError() << "fork failed";

    if (childPid == 0) try {
        // child
        if (setpgid(0, 0) == -1)
            ythrow TSystemError() << "setpgid";

        TFileHandle devNull(devNullStr, OpenExisting | WrOnly);

        // reenable SIGPIPE
        signal(SIGPIPE, SIG_DFL);

        // redirect output to the logFile
        TFileHandle stdinHandle(0);
        TFileHandle stdoutHandle(1);
        TFileHandle stderrHandle(2);

        if (!stdinHandle.LinkTo(devNull))
            ythrow TSystemError() << "dup2";

        if (!stdoutHandle.LinkTo(logFile))
            ythrow TSystemError() << "dup2";

        if (!stderrHandle.LinkTo(logFile))
            ythrow TSystemError() << "dup2";

        stdinHandle.Release();
        stdoutHandle.Release();
        stderrHandle.Release();
        devNull.Release();

        // close all shared file descriptors
        for (int fd = 3; fd < getdtablesize(); ++fd) {
#if 0 // Switched off code implemented bad way to preserve communism resources when restarting worker - see CLUSTERMASTER-52
            TVector<SOCKET>::iterator it = std::find(commSockets.begin(), commSockets.end(), fd);
            if (it != commSockets.end()) {   //communism socket
                continue;
            }
#endif
            close(fd);
        }

        // unblock signals
        sigset_t mask;
        SigFillSet(&mask);
        SigProcMask(SIG_UNBLOCK, &mask, nullptr);

        // block this two signals to correct work with cm cancel command
        SigEmptySet(&mask);
        SigAddSet(&mask, SIGTERM);
        SigAddSet(&mask, SIGINT);
        if (SigProcMask(SIG_BLOCK, &mask, nullptr)) {
            ythrow yexception() << "Can't block signals: " << LastSystemErrorText();
        }

        // run target
        execve(Type->Graph->WorkerGlobals->BinaryPath.data(), argv.data(), envp.data());

        yexception e;

        e << "execve fails: " << LastSystemErrorText() << "\n";
        e << "exec:";
        for (TVector<char*>::const_iterator i = argv.begin(); i != argv.end(); ++i)
            if (*i)
                e << " \"" << *i << "\"";
            else
                e << " NULL";
        e << "\n";

        for (TVector<char*>::const_iterator i = envp.begin(); i != envp.end(); ++i)
            e << "\"" << *i << "\"\n";

        throw e;
    } catch (const yexception& e) {
        Cerr << "Clustermaster error: " << e.what() << Endl;
        exit(1);
    }

    // parent: close log
    logFile.Close();

    LOG("START: Target " << Name << " Task " << nTask.GetN() << " pid " << childPid << " succeeded");

    return childPid;

#else

    return 0;

#endif
}

inline void TWorkerTarget::updateTaskResourceUsage(const TTargetTypeParameters::TId& nTask) {
    struct resUsageStat_st newStatistics;

    int err = Type->Graph->ResourceMonitor.getGroupStatistics(Tasks[nTask].GetPid(), &newStatistics);
    if (!err) {
        LOG("RES: Usage stat for " << Name << " " << Tasks[nTask].GetPid() <<
                " pid: CPU[M/A] = " << newStatistics.cpuMax << "/" << newStatistics.cpuAvg <<
                " MEM[M/A] = " << newStatistics.memMax << "/" << newStatistics.memAvg);
        if (newStatistics.sharedAreas->size()) {
            for (TMap<TString, double>::const_iterator i = newStatistics.sharedAreas->begin(); i != newStatistics.sharedAreas->end(); ++i) {
                LOG("RES: Shared files for " << Name << " " << Tasks[nTask].GetPid() << " pid: " <<
                        i->first << " = " << i->second);
            }
        }

        Tasks[nTask].updateAllResStat(newStatistics);
        reDefineResources(nTask);

        if (1 == Tasks[nTask].GetUpdateCounter()) {
            for (ui32 task = 0; task < Tasks.Size(); ++task) {
                ui32 taskUpdateCounter = Tasks.At(task).GetUpdateCounter();
                if (!taskUpdateCounter) {
                    Tasks.At(task).updateAllResStat(newStatistics);
                    reDefineResources(TTargetTypeParameters::TId(nTask.GetParameters(), task));
                }
            }
        }
    }
}

void TWorkerTarget::Finished(const TTargetTypeParameters::TId& nTask, TGraphChangeWatcher& watcher) {
    time_t finishTime = 0;
    int exitStatus = -1; // FIXME: handle unknown status without using -1

    try {
        NProto::TTargetExitStatuses statuses;
        TString exitStateFile = StateFilePathEx(Type->Graph->WorkerGlobals->VarDirPath, Name);

        TFile file(exitStateFile, OpenExisting | RdWr);
        file.Flock(LOCK_EX);
        TUnbufferedFileInput in(file);

        if (!statuses.ParseFromArcadiaStream(&in))
            ythrow yexception() << "cannot parse exit state from file";

        // if number of tasks had changed, forget state
        if (statuses.StatusSize() != Tasks.Size())
            ythrow yexception() << "task count mismatch";

        NProto::TTargetExitStatus* status = statuses.MutableStatus(nTask.GetN());
        finishTime = status->GetFinishedTime();
        exitStatus = status->GetExitStatus();

        status->SetFinishedTime(-1);
        status->SetExitStatus(-1);

        file.Seek(0, sSet);
        TUnbufferedFileOutput out(file);
        statuses.SerializeToArcadiaStream(&out);

        file.Flush();
    } catch (const yexception &e) {
        ERRORLOG("Error while loading target state: " << e.what());
    }

    if (exitStatus == -1) {    //target was brutally murdered
        finishTime = time(nullptr);
    }
    if (!Tasks[nTask].DoNotTrack())
        Tasks[nTask].SetLastFinished(finishTime);

    if (exitStatus != -1 && Tasks[nTask].GetState() == TS_CANCELING && WIFSIGNALED(exitStatus) && (WTERMSIG(exitStatus) == SIGTERM || WTERMSIG(exitStatus) == SIGKILL)) {
        LOG("FINISH: Target " << Name << " Task " << nTask.GetN() << " pid " << Tasks[nTask].GetPid() << " canceled");

        ChangeStateThroughRunning(watcher, nTask, Tasks[nTask].GetStateAfterCancel(), true);
    } else if (exitStatus != -1 && WIFEXITED(exitStatus) && WEXITSTATUS(exitStatus) == 0) {
        if (!Tasks[nTask].GetLostState() && !Tasks[nTask].DoNotTrack()) {
            updateTaskResourceUsage(nTask);
        }

        LOG("FINISH: Target " << Name << " Task " << nTask.GetN() << " pid " << Tasks[nTask].GetPid() << " succeeded");

        if (!Tasks[nTask].DoNotTrack()) {
            Tasks[nTask].SetLastSuccess(Tasks[nTask].GetLastFinished());
            Tasks[nTask].SetLastDuration(Tasks[nTask].GetLastSuccess() - Tasks[nTask].GetLastStarted());
        }

        ui32 toJump = Tasks[nTask].GetToRepeat();
        if ((toJump + 1 < jumpTimes) && conditionalJumpTargets.size()) {
            DEBUGLOG("Try to jumps " << conditionalJumpTargets.size() << "/" << jumpTimes);
            if (jumpTimes != JUMP_UNLIMITED) {
                Tasks[nTask].SetToRepeat(++toJump);
            }

            /*
             * WARN: Command propagating is used for cycle realization only at the moment. It's the brute hack so
             * you need to pay attention to if propagating will be used for other purposes
             */
            ChangeState(watcher, nTask, TS_IDLE, false);
            for (TList<TString>::const_iterator follower = conditionalJumpTargets.begin();
                    follower != conditionalJumpTargets.end(); ++follower) {
                if (*follower != Name)
                {
                    TCommandMessage msg(*follower, -1, TCommandMessage::CF_INVALIDATE, TS_SUCCESS);
                    Singleton<TMasterMultiplexor>()->SendToAll(msg);
                    DEBUGLOG("    Invalidate " << *follower);
                }
            }

            TCommandMessage msg(Name, -1, TCommandMessage::CF_RECURSIVE_UP | TCommandMessage::CF_RUN, TS_IDLE);
            Singleton<TMasterMultiplexor>()->SendToAll(msg);
        } else {
            /* Should be always ++0 for unrepeatable tasks */
            Tasks[nTask].SetToRepeat(++toJump);
            ChangeStateThroughRunning(watcher, nTask, TS_SUCCESS, true);
            for (TDependsList::iterator follower = Followers.begin(); follower != Followers.end(); ++follower)
                if (follower->IsLocal())
                    follower->GetTarget()->TryReadySomeTasks(watcher);

            if (Semaphore.Get() != nullptr && Semaphore->IsLast(this)) {
                Semaphore->Update();
                Semaphore->TryReadySomeTasks(watcher);
            }
        }
    } else {
        TString exitCode;
        if (WIFEXITED(exitStatus)) {
            exitCode = Sprintf(", exit code %d", WEXITSTATUS(exitStatus));
        } else if (WIFSIGNALED(exitStatus)) {
            exitCode = Sprintf(", got signal %s", strsignal(WTERMSIG(exitStatus)));
        }
        LOG("FINISH: Target " << Name << " Task " << nTask.GetN() << " pid " << Tasks[nTask].GetPid() << " FAILED" << exitCode);

        Tasks[nTask].SetLastFailure(Tasks[nTask].GetLastFinished());

        ChangeStateThroughRunning(watcher, nTask, TTaskState(TS_FAILED, WIFSIGNALED(exitStatus) ? WTERMSIG(exitStatus) : 0), true);

        TTargetTypeParametersMap<bool> mask(&Type->GetParametersHyperspace());
        mask.At(GetHyperspaceTaskIndex(nTask.GetN())) = true;
        Type->Graph->Depfail(this, mask, watcher, true);

        struct tm tm;
        if (!Type->Graph->WorkerGlobals->ArchiveDirPath.empty())
            system(Sprintf("gzip < %s/%s.%03d.log > %s/%s.%03d.%s.log.gz &",
                        Type->Graph->WorkerGlobals->LogDirPath.data(), Name.data(), int(nTask.GetN()),
                        Type->Graph->WorkerGlobals->ArchiveDirPath.data(), Name.data(), int(nTask.GetN()),
                        Strftime("%Y-%m-%dT%H:%M:%S", TInstant::Now().LocalTime(&tm)).data()
                    ).data());
    }
    if (!Tasks[nTask].DoNotTrack())
        Type->Graph->ResourceMonitor.rmProcessGroup(Tasks[nTask].GetPid());
    Tasks[nTask].SetPid(-1);
    Tasks[nTask].SetLostState(false);
}

void TWorkerTarget::Stopped(const TTargetTypeParameters::TId& nTask, TGraphChangeWatcher& watcher) {
    if (Tasks[nTask].GetState() == TS_STOPPING) {
        LOG("Target " << Name << " Task " << nTask.GetN() << " pid " << Tasks[nTask].GetPid() << " stopped");

        ChangeState(watcher, nTask, TS_STOPPED);
    } else {
        Y_FAIL("Impossible state");
    }
}

void TWorkerTarget::Continued(const TTargetTypeParameters::TId& nTask, TGraphChangeWatcher& watcher) {
    if (Tasks[nTask].GetState() == TS_CONTINUING) {
        LOG("Target " << Name << " Task " << nTask.GetN() << " pid " << Tasks[nTask].GetPid() << " continued");

        ChangeState(watcher, nTask, TS_RUNNING);
    } else {
        Y_FAIL("Impossible state");
    }
}

void TWorkerTarget::TryReadySomeTasks(TGraphChangeWatcher& watcher) {
    if (!Type->IsOnThisWorker())
        return;

    if (HasCrossnodeDepends)
        return;

    if (Semaphore.Get() != nullptr)
        Semaphore->Update();

    const TTaskState& newState = ReadyToGoStateOrSuspended();

    TTargetTypeParametersMap<bool> notReadyToGo(&Type->GetParameters());

    for (TDependsList::const_iterator depend = Depends.begin(); depend != Depends.end(); ++depend) {
        Y_VERIFY(depend->IsLocal(), "Must be local");

        // skip inactive conditional depends
        if (!depend->GetCondition().IsEmpty() && !Type->Graph->GetVariables().CheckCondition(depend->GetCondition())) {
           continue;
        }

        // copy-paste from master
        IPrecomputedTaskIdsInitializer* precomputed = depend->GetPrecomputedTaskIdsMaybe()->Get();
        // precomputed->Initialize();

        for (TDependEdgesEnumerator en(precomputed->GetIds()); en.Next(); ) {
            for (TPrecomputedTaskIdsForOneSide::TIterator
                    depTask = en.GetDepTaskIds().Begin();
                    depTask != en.GetDepTaskIds().End();
                    ++depTask)
            {
                const TSpecificTaskStatus *depSpecStatus = &depend->GetTarget()->Tasks.At(*depTask);
                if (depSpecStatus->GetState() != TS_SUCCESS && depSpecStatus->GetState() != TS_SKIPPED) {
                    for (TPrecomputedTaskIdsForOneSide::TIterator myTask = en.GetMyTaskIds().Begin(); myTask != en.GetMyTaskIds().End(); ++myTask) {
                        notReadyToGo.At(*myTask) = true;
                    }
                }
            }
        }
    }

    for (TTargetTypeParametersMap<bool>::TConstEnumerator notReadyTask = notReadyToGo.Enumerator();
            notReadyTask.Next(); )
    {
        TWorkerTaskStatus& task = GetTasks().At(notReadyTask.CurrentN());
        if (!*notReadyTask && task.GetState() == TS_PENDING) {
            if (Semaphore.Get() == nullptr || Semaphore->TryRun(notReadyTask.CurrentN())) {
                ChangeState(watcher, notReadyTask.CurrentN(), newState);
            }
        }
    }
}

bool TWorkerTarget::Kill(const TTargetTypeParameters::TId& nTask, ui32 flags) {
    return SendSignal(nTask, (flags & TCommandMessage::CF_KILL) ? SIGKILL : SIGTERM, true);
}

bool TWorkerTarget::Stop(const TTargetTypeParameters::TId& nTask) {
    return SendSignal(nTask, SIGSTOP);
}

bool TWorkerTarget::Continue(const TTargetTypeParameters::TId& nTask) {
    return SendSignal(nTask, SIGCONT);
}

bool TWorkerTarget::SendSignal(const TTargetTypeParameters::TId& nTask, int signal, bool removeProcessGroup) {
    if (Tasks[nTask].GetPid() == -1) {
        ERRORLOG1(target, Name << " task " << nTask.GetN() << ": killing, but there's no pid");
        return false;
    }

    DEBUGLOG1(target, "Sending signal " << signal << " to " << Name << " task " << nTask.GetN() << ", pid=" << Tasks[nTask].GetPid());
    if (removeProcessGroup)
        Type->Graph->ResourceMonitor.rmProcessGroup(Tasks[nTask].GetPid());

#ifndef _win_

    // Att: kills whole process group
    if (kill(-Tasks[nTask].GetPid(), signal) == -1) {
        ERRORLOG1(target, Name << " task " << nTask.GetN() << ": Kill failed: " << LastSystemErrorText());
//        ythrow yexception() << "kill: " << LastSystemErrorText();
        return false;
    }

#endif

    return true;
}

void TWorkerTarget::BumpPriority(float priority, TTraversalGuard& guard, int flags) {
    if (!guard.TryVisit(this))
        return;

    if (priority > Priority)
        Priority = priority;

    if (flags & UP)
        for (TDependsList::iterator depend = Depends.begin(); depend != Depends.end(); ++depend)
            depend->GetTarget()->BumpPriority(priority, guard, flags);

    if (flags & DOWN)
        for (TDependsList::iterator follower = Followers.begin(); follower != Followers.end(); ++follower)
            follower->GetTarget()->BumpPriority(priority, guard, flags);
}

void TWorkerTarget::CommandOnTask(TTargetTypeParameters::TId id, ui32 flags, TGraphChangeWatcher& watcher) {
    TSpecificTaskStatus *task = &(Tasks.At(id));

    bool statChanged = false;

    TTaskState newstate = task->GetState();
    TTaskState oldstate = newstate;

    // Run
    if (flags & TCommandMessage::CF_RUN) {
        if (task->GetPid() == -1) {
            if (task->GetState() != TS_SUSPENDED) {
                if (flags & TCommandMessage::CF_FORCE_RUN) {
                    newstate = TS_RUNNING;
                } else if (flags & TCommandMessage::CF_FORCE_READY) {
                    newstate = ReadyToGoStateOrSuspended();
                } else if (flags & TCommandMessage::CF_RETRY) {
                    if (task->GetState() == TS_FAILED || task->GetState() == TS_DEPFAILED)
                        newstate = TS_PENDING;
                } else if (flags & TCommandMessage::CF_MARK_SKIPPED) {
                    if (task->GetState() != TS_PENDING && task->GetState() != TS_READY && task->GetState() != TS_SUCCESS)
                        newstate = TS_SKIPPED;
                } else if (flags & TCommandMessage::CF_IDEMPOTENT) {
                    if (task->GetState() != TS_FAILED && task->GetState() != TS_DEPFAILED &&
                        task->GetState() != TS_READY && task->GetState() != TS_SUCCESS)
                            newstate = TS_PENDING;
                } else if (task->GetState() != TS_READY && task->GetState() != TS_SUCCESS) {
                    newstate = TS_PENDING;
                }
            } else {
                if (flags & TCommandMessage::CF_FORCE_RUN) {
                    newstate = TS_RUNNING;
                }
            }
        } else {
            if (task->GetState() == TS_STOPPED && (flags & TCommandMessage::CF_FORCE_RUN)) {
                if (Continue(id)) {
                    newstate = TS_CONTINUING;
                } else {
                    newstate = TS_FAILED;
                }
            }
        }
    }

    // Cancel
    if (flags & TCommandMessage::CF_CANCEL) {
        auto state = (flags & TCommandMessage::CF_INVALIDATE) ? TS_IDLE : TS_CANCELED;

        task->SetToRepeat(0);
        if (task->GetPid() != -1) {
            if (!task->IsStopped()) { // do not touch stopped task as kill will do nothing for stopped process
                if (Kill(id, flags)) {
                    newstate = TS_CANCELING;
                    task->SetStateAfterCancel(state);
                } else {
                    newstate = state;
                    task->SetPid(-1);
                }
            }
        } else if (task->GetState() == TS_READY || task->GetState() == TS_PENDING || task->GetState() == TS_SUSPENDED) {
            newstate = state;
        }
    }

    // Invalidate
    if ((flags & TCommandMessage::CF_INVALIDATE) && (task->GetPid() == -1)) {
        if (!(flags & TCommandMessage::CF_REMAIN_SUCCESS && task->GetState() == TS_SUCCESS)) {
            newstate = TS_IDLE;
        }

        if (conditionalJumpTargets.size()) { /* It's meaningful only for inner cycles. */
            task->SetToRepeat(0);
        }
    }

    // Mark successful
    if ((flags & TCommandMessage::CF_MARK_SUCCESS) && (task->GetPid() == -1))
        newstate = TS_SUCCESS;

    // Reset statistics
    if ((flags & TCommandMessage::CF_RESET_STAT)) {
        LOG("Reset resource stat for " << Name);
        task->ResetResourceStatistics();
        reDefineResources(id);
        statChanged = true;
    }

    if (oldstate != newstate)
        ChangeState(watcher, id, newstate, statChanged);
}

TAutoPtr<TThinStatusMessage> TWorkerTarget::CreateThinStatus() const {
    TAutoPtr<TThinStatusMessage> message(new TThinStatusMessage);

    message->SetName(Name);

    for (TTaskList::TConstEnumerator task = Tasks.Enumerator(); task.Next(); ) {
        message->AddState(task->GetState().GetId()); // obsoleted. see messages.proto

        NProto::TThinTaskStatus* const status = message->AddStatus();
        status->SetState(task->GetState().GetId());
        status->SetStateValue(task->GetState().GetValue());
        status->SetLastChanged(task->GetLastChanged());
    }

    return message;
}

TAutoPtr<TSingleStatusMessage> TWorkerTarget::CreateSingleStatus(TTargetTypeParameters::TId nTask) const {
    TAutoPtr<TSingleStatusMessage> message(new TSingleStatusMessage);

    message->SetName(Name);
    message->SetTask(nTask.GetN());

    const TWorkerTaskStatus* task = &Tasks.At(nTask);

    propagateStatistics(message->MutableStatus(), task);
    message->MutableStatus()->SetRepeatMax(jumpTimes);
    return message;
}

void TWorkerTarget::RegisterDependency(TWorkerTarget* depend, TTargetParsed::TDepend& dependParsed, const TTargetGraphBase<TWorkerGraphTypes>::TParserState& state,
        TWorkerGraphTypes::TPrecomputedTaskIdsContainer* precomputedTaskIdsContainer)
{
#if 0
    // must always build graph because of recursive_up command
    if (!Type->IsOnThisWorker()) {
        return;
    }

    if (HasCrossnodeDepends) {
        return;
    }
#endif

    TTargetBase<TWorkerGraphTypes>::RegisterDependency(depend, dependParsed, state, precomputedTaskIdsContainer);

    // Retrieving normal and invert edges

    TWorkerDepend &normal = Depends.back();
    TWorkerDepend &invert = depend->Followers.back();

    // Initializing PrecomputedTaskIds for local edges

    TSimpleSharedPtr<TPrecomputedTaskIdsMaybe<TWorkerGraphTypes> > taskIdsMaybe;
    if (normal.IsLocal()) {
        Y_VERIFY(invert.IsLocal(), "Both normal and invert edges must be local");
        Y_VERIFY(normal.GetMode() == DM_PROJECTION, "Mode of local edges could be only DM_PROJECTION"); // This assertion is implicit but is
                // right. IsLocal() returns true only if source target has no crossnode depends and group depend is always crossnode.
        TSimpleSharedPtr<IPrecomputedTaskIdsInitializer> initializer = precomputedTaskIdsContainer->Container.GetOrCreate(
                TDependSourceTypeTargetTypeAndMappings<TWorkerTargetType>(*normal.GetJoinParamMappings(), normal.GetSource()->Type, normal.GetTarget()->Type));
        initializer->Initialize();
        taskIdsMaybe.Reset(TPrecomputedTaskIdsMaybe<TWorkerGraphTypes>::Defined(initializer));
    }
    normal.SetPrecomputedTaskIdsMaybe(taskIdsMaybe);
    invert.SetPrecomputedTaskIdsMaybe(taskIdsMaybe);

    // Initializing PrecomputedHyperspaceTaskIds for all edges

    TSimpleSharedPtr<TPrecomputedTaskIdsMaybe<TWorkerGraphTypes> > taskIdsMaybeHyperspace;

    if (normal.GetMode() == DM_PROJECTION) {
        TSimpleSharedPtr<TPrecomputedTaskIdsInitializerHyperspace> initializer = precomputedTaskIdsContainer->ContainerHyperspace.GetOrCreate(
                TDependSourceTypeTargetTypeAndMappings<TWorkerTargetType>(*normal.GetJoinParamMappingsHyperspace(), normal.GetSource()->Type, normal.GetTarget()->Type));
        initializer->Initialize();
        taskIdsMaybeHyperspace.Reset(TPrecomputedTaskIdsMaybe<TWorkerGraphTypes>::Defined(initializer));
    } else if (normal.GetMode() == DM_GROUPDEPEND) {
        TSimpleSharedPtr<TWorkerPrecomputedTaskIdsInitializerGroup> initializer = precomputedTaskIdsContainer->ContainerGroup.GetOrCreate(
                TWorkerDependSourceTypeTargetType(normal.GetSource()->Type, normal.GetTarget()->Type));
        initializer->Initialize();
        taskIdsMaybeHyperspace.Reset(TPrecomputedTaskIdsMaybe<TWorkerGraphTypes>::Defined(initializer));
    } else {
        Y_FAIL("Bad depend mode");
    }

    normal.SetPrecomputedTaskIdsMaybeHyperspace(taskIdsMaybeHyperspace);
    invert.SetPrecomputedTaskIdsMaybeHyperspace(taskIdsMaybeHyperspace);

    if (dependParsed.Flags & DF_SEMAPHORE) {
        if (depend->Semaphore.Get() == nullptr) {
            depend->Semaphore = new TSemaphore(depend);
            Semaphore = depend->Semaphore;
        } else {
            Semaphore = depend->Semaphore;
        }

        Semaphore->AddTarget(this);
    }
}

void TWorkerTarget::formatRequestFromStatistics(const TTargetTypeParameters::TId& nTask) {
    const resUsageStat_st& taskResUsage = Tasks[nTask].GetResStat();

    TResourcesStruct::const_iterator it = Tasks.At(nTask).Resources.begin();
    while (it != Tasks.At(nTask).Resources.end()) {
        if (it->Auto) {
            Tasks.At(nTask).Resources.erase(it++);
        } else {
            ++it;
        }
    }

    const std::pair<TResourcesStruct::iterator, bool> cpu = Tasks.At(nTask).Resources.Insert("cpu");
    if (cpu.second || cpu.first->Auto) {
        cpu.first->Auto = true;
        double cpuToAcquire = (taskResUsage.cpuAvg + taskResUsage.cpuMax) / 2 ;
        if (cpuToAcquire < 0.01)
            cpu.first->Val = 0.0;
        else
            cpu.first->Val = Min<double>(cpuToAcquire, 0.99);
    }

    double totalMemAck = 0.00;
    const std::pair<TResourcesStruct::iterator, bool> mem = Tasks.At(nTask).Resources.Insert("mem");
    if (mem.second || mem.first->Auto) {
        mem.first->Auto = true;

        if (taskResUsage.memMax < 0.01)
            mem.first->Val = 0.0;
        else
            mem.first->Val = Min<double>(taskResUsage.memMax, 0.99);

        totalMemAck += mem.first->Val;
    }

    /* Try to not count shared mem at all. Let's loook.. */
    #if 0
    if (!!taskResUsage.sharedAreas) {
        for (TMap<TString, double>::const_iterator i = taskResUsage.sharedAreas->begin(); i != taskResUsage.sharedAreas->end(); ++i) {
            const std::pair<TResourcesStruct::iterator, bool> shmem = Tasks.At(nTask).Resources.Insert("mem", TString(i->first));
            if (shmem.second || shmem.first->Auto) {
                shmem.first->Auto = true;

                if (i->second < 0.01)
                    shmem.first->Val = 0.0;
                else
                    shmem.first->Val = Min<double>(i->second, 0.99);

                if (totalMemAck + shmem.first->Val > 1.00) { /* Shouldn't be happend. */
                    shmem.first->Val = 0.99 - totalMemAck;
                    LOG("RES: :ERROR: Auto planing of shared mem is incorrect! Target " << Name);
                    break;
                }
                totalMemAck += shmem.first->Val;
            }
        }
    }
    #endif
}

void TWorkerTarget::reDefineResources(const TTargetTypeParameters::TId& nTask) {
    if (DoUseAnyResource && Tasks[nTask].ResState == RS_CONNECTED && Tasks.At(nTask).Resources.GetAuto()) {
        if (Tasks.At(nTask).Resources.GetAuto())
            formatRequestFromStatistics(nTask);
        GetResourceManager().Define(
                TResourcesRequest(Name, nTask.GetN()),
                Tasks.At(nTask).Resources.GetSolver(),
                *Tasks.At(nTask).Resources.CompoundDefinition());
        GetResourceManager().Details(
                TResourcesRequest(Name, nTask.GetN()),
                NCommunism::TDetails().SetDuration(TDuration::Seconds(Tasks[nTask].GetLastDuration())));
    }
}

void TWorkerTarget::DefineResources(TGraphChangeWatcher& watcher) {
    Y_VERIFY(&Type->GetParameters() == Tasks.GetParameters(), "useful assertion");

    ResourcesUseVariables = false;

    if (DoUseAnyResource)
    for (TTaskList::TEnumerator task = Tasks.Enumerator(); task.Next(); ) {
        TTargetTypeParameters::TId nTask = task.CurrentN();

        Type->GetParameters().CheckId(nTask);

        TString resourcesString(ResourcesString);

        SubstituteNumericArg(resourcesString, 0, TString(HostName(), 0, HostName().find('.')));

        TVector<TString> args = Type->GetParameters().GetScriptArgsOnWorkerByN(nTask);
        for (size_t i = 0; i < args.size(); ++i) {
            SubstituteNumericArg(resourcesString, i + 1, args.at(i));
        }

        try {
            const TWorkerVariables::TMapType::TVar::TRevision revision = Type->Graph->GetVariables().Substitute(resourcesString);
            ResourcesUseVariables = revision || ResourcesUseVariables;
            if (ResourcesUseVariables && revision != Type->Graph->GetVariables().GetLastRevision())
                continue;
        } catch (const TWorkerVariables::TNoVariable& e) {
            ResourcesUseVariables = true;

            const TString message = TString("variable ") + e.what() + " not set";
            Tasks.At(nTask).SetResState(RS_INCOMPLETE, message);
            DEBUGLOG1(target, Name << ": Task " << nTask.GetN() << ": incomplete resources definition: " << message);
            continue;
        }

        try {
            Tasks.At(nTask).Resources.Parse(resourcesString, Type->Graph->WorkerGlobals->DefaultResourcesHost);
        } catch (const TResourcesIncorrect& e) {
            const TString message = e.what();
            Tasks.At(nTask).SetResState(RS_INCORRECT, message);
            LOG1(target, Name << ": Task " << nTask.GetN() << ": incorrect resources definition: " << message);
            continue;
        }

        if (Tasks.At(nTask).Resources.GetAuto())
            formatRequestFromStatistics(nTask);

        GetResourceManager().Define(
                TResourcesRequest(Name, nTask.GetN()),
                Tasks.At(nTask).Resources.GetSolver(),
                *Tasks.At(nTask).Resources.CompoundDefinition());

        NCommunism::TDetails details;

        details.SetPriority(Type->Graph->WorkerGlobals->PriorityRange.first * (1.0f - Priority) + Type->Graph->WorkerGlobals->PriorityRange.second * Priority);
        details.SetLabel(Name + "/" + ToString(nTask.GetN()));

        if (Tasks.At(nTask).GetLastStarted())
            details.SetDuration(TDuration::Seconds(Tasks[nTask].GetLastDuration()));

        GetResourceManager().Details(TResourcesRequest(Name, nTask.GetN()), details);

        if (Tasks.At(nTask).ResState != RS_CONNECTED)
            Tasks.At(nTask).SetResState(RS_CONNECTING);

        watcher.Register(this, nTask, Tasks.At(nTask).GetState(), Tasks.At(nTask).GetState(), false);
    }
}

void TWorkerTarget::UndefResources() {
    for (TResourcesRequest request(Name, 0); request.second < Tasks.Size(); ++request.second)
        if (Tasks[request.second].ResState != RS_DEFAULT) {
            GetResourceManager().Undef(request);
            Tasks[request.second].SetResState(RS_DEFAULT);
        }
}

void TWorkerTarget::RequestResources(TTargetTypeParameters::TId nTask) const {
    if (Tasks[nTask].ResState == RS_CONNECTED || Tasks[nTask].ResState == RS_CONNECTING) {
        GetResourceManager().Request(TResourcesRequest(Name, nTask.GetN()));
        DEBUGLOG1(target, Name << ": " << nTask.GetN() << ' ' << ToString<EResourcesState>(Tasks[nTask].ResState) << ": REQUEST: ok");
    }
}

void TWorkerTarget::ClaimResources(TTargetTypeParameters::TId nTask) const {
    while (true) {
        try {
            GetResourceManager().Claim(TResourcesRequest(Name, nTask.GetN()), true);
        } catch (const NCommunism::TNotDefined&) {
            try {
                GetResourceManager().SetCustomEvent(TResourceManager::TEvent(TResourcesRequest(Name, nTask.GetN()), NCommunism::GRANTED, TString()));
                GetResourceManager().Signal();
            } catch (const NCommunism::TDefined&) {
                continue;
            }
        }
        break;
    }
    DEBUGLOG1(target, Name << ": " << nTask.GetN() << ' ' << ToString<EResourcesState>(Tasks[nTask.GetN()].ResState) << ": CLAIM: ok");
}

void TWorkerTarget::DisclaimResources(TTargetTypeParameters::TId nTask) const {
    try {
        GetResourceManager().Disclaim(TResourcesRequest(Name, nTask.GetN()));
    } catch (const NCommunism::TNotDefined&) {
        return;
    }
    DEBUGLOG1(target, Name << ": " << nTask.GetN() << ' ' << ToString<EResourcesState>(Tasks[nTask].ResState) << ": DISCLAIM: ok");
}

void TWorkerTarget::ProcessResourcesEvent(const TResourceManager::TEvent& event) {
    const TTargetTypeParameters::TId nTask = Type->GetParameters().GetId(event.Userdata.second);
    TWorkerTaskStatus* task = &Tasks.At(nTask);

    switch (event.Status) {
    case NCommunism::BADVERSION:
        DEBUGLOG1(target, Name << ": Task " << nTask.GetN() << " bad version");
        task->SetResState(RS_BADVERSION, event.Message);
        break;
    case NCommunism::CONNECTED:
        DEBUGLOG1(target, Name << ": Task " << nTask.GetN() << " connected");
        task->SetResState(RS_CONNECTED, event.Message);
        break;
    case NCommunism::RECONNECTING:
        DEBUGLOG1(target, Name << ": Task " << nTask.GetN() << " reconnecting");
        task->SetResState(RS_CONNECTING, event.Message);
        break;
    case NCommunism::GRANTED: {
        DEBUGLOG1(target, Name << ": Task " << nTask.GetN() << " granted");

        TGraphChangeWatcher watcher;

        if (task->GetPid() == -1) try {
            Type->Graph->TaskStarted(this, nTask.GetN(), TryToRun(nTask, watcher));
        } catch (const yexception& e) {
            ERRORLOG1(target, Name << ": Task running failed: " << e.what());
        }

        watcher.Commit();
        break;
    } case NCommunism::EXPIRED:
        Y_FAIL("unexpected EXPIRED resource event");
        break;
    case NCommunism::REJECTED:
        DEBUGLOG1(target, Name << ": Task " << nTask.GetN() << " rejected");
        task->SetResState(RS_REJECTED, event.Message);
        break;
    }
}

void TWorkerTarget::CheckExpiredTasks(TGraphChangeWatcher& watcher) {
    time_t now = time(nullptr);

    for (TTaskList::TEnumerator task = Tasks.Enumerator(); task.Next(); ) {
        if (task->GetState() == TS_RUNNING && Timeout > 0 && now > task->GetLastStarted() + Timeout) {
            LOG1(target, Name << ": Task " << task.CurrentN().GetN() << " timed out (" << Timeout << " seconds), killing");
            if (Kill(task.CurrentN(), 0)) {
                task->SetStateAfterCancel(TS_FAILED);
                ChangeState(watcher, task.CurrentN(), TS_CANCELING);
            } else {
                ChangeState(watcher, task.CurrentN(), TS_FAILED);
            }
        }
    }
}

void TWorkerTarget::StopTasks(TGraphChangeWatcher& watcher) {
    for (TTaskList::TEnumerator task = Tasks.Enumerator(); task.Next(); ) {
        if (task->GetState() == TS_RUNNING) {
            if (Stop(task.CurrentN()))
                ChangeState(watcher, task.CurrentN(), TS_STOPPING);
            else
                ChangeState(watcher, task.CurrentN(), TS_FAILED);
        } else if (task->GetState() == TS_READY) {
            ChangeState(watcher, task.CurrentN(), TS_SUSPENDED); // resources should be disclaimed by state watcher
        }
    }
}

void TWorkerTarget::ContinueTasks(TGraphChangeWatcher& watcher) {
    for (TTaskList::TEnumerator task = Tasks.Enumerator(); task.Next(); ) {
        if (task->GetState() == TS_STOPPED) {
            if (Continue(task.CurrentN())) {
                ChangeState(watcher, task.CurrentN(), TS_CONTINUING);
            } else {
                ChangeState(watcher, task.CurrentN(), TS_FAILED);
            }
        } else if (task->GetState() == TS_SUSPENDED) {
            ChangeState(watcher, task.CurrentN(), ReadyToGoState());
        }
    }
}

void TWorkerTarget::CheckState() const {
    TTargetBase<TWorkerGraphTypes>::CheckState();
}

void TWorkerTarget::DumpStateExtra(TPrinter& out) const {
    if (!Type->IsOnThisWorker()) {
        out.Println("not on this worker");
        return;
    }
    TPrinter l1 = out.Next();
    for (TConstTaskIterator task = TaskIterator(); task.Next(); ) {
        l1.Println(ToString(task.GetPath())
                + " " + ToString(task->GetState())
                );
    }
}

bool TWorkerTarget::GetNoRestartAfterLoss() const {
    return NoRestartAfterLoss;
}

