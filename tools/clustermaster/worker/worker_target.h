#pragma once

#include "precomputed_task_ids_hyperspace.h"
#include "precomputed_task_ids_hyperspace_group.h"
#include "resource_manager.h"
#include "resource_monitor.h"
#include "semaphore.h"
#include "worker.h"
#include "worker_list_manager.h"
#include "worker_target_graph_types.h"
#include "worker_target_type.h"

#include <tools/clustermaster/common/target_impl.h>
#include <tools/clustermaster/common/target_type_parameters_map.h>
#include <tools/clustermaster/communism/client/client.h>
#include <tools/clustermaster/proto/target.pb.h>

#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

class TWorkerGraph;
class TGraphChangeWatcher;
struct TThinStatusMessage;
struct TSingleStatusMessage;

namespace LogSubsystem {
    class target {};
}

struct TWorkerPrecomputedTaskIdsContainer {
    TPrecomputedTaskIdsContainer<TWorkerGraphTypes, TParamsByTypeOrdinary> Container;
    TPrecomputedTaskIdsContainerHyperspace ContainerHyperspace;
    TWorkerPrecomputedTaskIdsContainerGroup ContainerGroup;
};

class TWorkerTaskStatus: public TTaskStatus {
public:
    EResourcesState ResState;
    TString ResMessage;
    TTaskState StateAfterCancel;
    bool Lost; // flag defines that this task was started not from this worker's instance, i.e. this task was lost
    TResourcesStruct Resources;

    TWorkerTaskStatus()
        : ResState(RS_DEFAULT)
        , StateAfterCancel(TS_IDLE)
        , Lost(false)
    {
    }

    const TTaskState& GetStateAfterCancel() const noexcept { return StateAfterCancel; }
    void SetStateAfterCancel(const TTaskState& s) noexcept { StateAfterCancel = s; }

    void SetResState(EResourcesState resState, const TString& resMessage = TString()) { ResState = resState; ResMessage = resMessage; }

    bool GetLostState() const { return Lost; }
    void SetLostState(bool lost) { Lost = lost; }
};


class TWorkerTarget: public TTargetBase<TWorkerGraphTypes>, TNonCopyable {
public:
    typedef TWorkerTargetType::TTaskIdSafe TTaskIdSafe;

    enum BumpPriorityFlags {
        UP = 0x01,
        DOWN = 0x02,
    };

private:
    TTaskStatuses SerializeStateToProtobuf() const;
    const TTaskState& ReadyToGoState() const;
    const TTaskState& ReadyToGoStateOrSuspended() const;
    const TFsPath GetStateFilePath() const;

public:
    TWorkerTarget(const TString& n, TWorkerTargetType* t, const TParserState&);

    ~TWorkerTarget() override;

    void AddOption(const TString &key, const TString &value) override;

    bool IsSame(const TWorkerTarget* right) const override;
    void SwapState(TWorkerTarget* right) override;
    void LoadState() override;
    void SaveStateToYt(const TTaskStatuses& protoState) const;
    void SaveStateToDisk(const TTaskStatuses& protoState) const;
    void SaveState() const override;

    // Execution control
    void PokeReady(const TMaybe<TTargetTypeParameters::TId>& nTask, TGraphChangeWatcher& watcher);
    void PokeReadyAllTasks(TGraphChangeWatcher& watcher);

    /**
     * @brief Check task with such nTaks was run already. If task was run already in the other worker's instance,
     *        just make some little work and return, don't start another process for this task. If task was not run,
     *        run it!
     * @param nTask
     * @param watcher
     * @return pid value of new started process, or lost process, if it already was run
     */
    pid_t TryToRun(const TTargetTypeParameters::TId& nTask, TGraphChangeWatcher& watcher);
    void Finished(const TTargetTypeParameters::TId& nTask, TGraphChangeWatcher& watcher);
    void Stopped(const TTargetTypeParameters::TId& nTask, TGraphChangeWatcher& watcher);
    void Continued(const TTargetTypeParameters::TId& nTask, TGraphChangeWatcher& watcher);

    bool SendSignal(const TTargetTypeParameters::TId& nTask, int signal, bool removeProcessGroup = false);
    bool Kill(const TTargetTypeParameters::TId& nTask, ui32 flags);
    bool Stop(const TTargetTypeParameters::TId& nTask);
    bool Continue(const TTargetTypeParameters::TId& nTask);

    void ChangeState(TGraphChangeWatcher& watcher, TTargetTypeParameters::TId nTask, const TTaskState& newState, bool timesChanged = false);
    void ChangeStateThroughRunning(TGraphChangeWatcher& watcher, TTargetTypeParameters::TId nTasl, const TTaskState& newState,
                                   bool timesChanged = false);
    void TryReadySomeTasks(TGraphChangeWatcher& watcher);
    bool CheckSemaphore(int nTask);

    void CheckExpiredTasks(TGraphChangeWatcher& watcher);

    void StopTasks(TGraphChangeWatcher& watcher);
    void ContinueTasks(TGraphChangeWatcher& watcher);

    // Recursive status changes
    void BumpPriority(float priority, TTraversalGuard& guard, int flags);

    // Command processing
    void CommandOnTask(TTargetTypeParameters::TId nTask, ui32 flags, TGraphChangeWatcher& watcher);

    void ExportThinStatus(TThinStatusMessage *message) const;

    void RegisterDependency(TWorkerTarget* depend, TTargetParsed::TDepend&, const TTargetGraphBase<TWorkerGraphTypes>::TParserState&,
            TWorkerGraphTypes::TPrecomputedTaskIdsContainer* precomputedTaskIdsContainer) override;

    typedef NCommunism::TNotDefined TResourcesNotDefined;

    void DefineResources(TGraphChangeWatcher& watcher);
    void UndefResources();
    void RequestResources(TTargetTypeParameters::TId nTask) const;
    void ClaimResources(TTargetTypeParameters::TId nTask) const;
    void DisclaimResources(TTargetTypeParameters::TId nTask) const;

    void ProcessResourcesEvent(const TResourceManager::TEvent& event);

    TAutoPtr<TThinStatusMessage> CreateThinStatus() const;
    TAutoPtr<TSingleStatusMessage> CreateSingleStatus(TTargetTypeParameters::TId nTask) const;

public:

    typedef TMap<TString, TString> TOptionsMap;

    void SetPriority(float prio);
    inline float GetPriority () const { return Priority; }
    const TOptionsMap& GetOptions() const noexcept { return UserOptions; }
    const TString& GetResourcesString() const noexcept { return ResourcesString; }
    bool GetResourcesUseVariables() const noexcept { return ResourcesUseVariables; }
    bool IsUsingResources() const noexcept { return DoUseAnyResource; }
    void GetFollowers(TSet<TString> const& notFollowThrew, TSet<TString>* allFollowers);
    void GetDependers(TSet<TString> const& notFollowThrew, TSet<TString>* allDependers);
    ui32 GetJumpTimes() const { return jumpTimes; }

    bool GetDontStop() const { return DontStop; }

    void SetAllTaskState(const TTaskState& state);

    /**
     * @brief Check that task already run
     * @param pid Pid value of running task
     * @param startedTime Time since Epoch current task with pid was started
     * @param procStartedTime Time from /proc current task with pid was started
     * @return true, if task was run and is running now
     *         false, if task was run, but for current time it is dead
     */
    bool CheckTaskAvailability(pid_t pid, ui32 startedTime, ui32 procStartedTime);
    /**
     * @brief Get process start time directly from /proc
     * @param pid Pid value of running task
     * @return process start time in secs as mentioned in /proc/<pid>/stat or /proc/<pid>/status
     *         (time_t)-1 on error
     */
    time_t GetProcStartTime(pid_t pid);
    /**
     * @brief Get process start time since Epoch directly from /proc
     * @param pid Pid value of running task
     * @return process start time in secs since Epoch,
     *         (time_t)-1 on error
     */
    time_t GetStartTime(pid_t pid);

    TMaybe<TTaskStatuses> LoadTargetStatusesFromYt() const;
    TMaybe<TTaskStatuses> LoadTargetStatusesFromDisk() const;
    TMaybe<TTaskStatuses> LoadTargetStatuses() const;

    size_t GetHyperspaceTaskIndex(size_t localspaceTaskIndex) const;

private:
    /* Save options for target. Apply this options lately for whole graph */
    TOptionsMap UserOptions;

    TSimpleSharedPtr<TSemaphore> Semaphore;

    TString ResourcesString;

    bool DoUseAnyResource;
    bool ResourcesUseVariables;

    int Timeout;
    float Priority;

    /* Possibility to make a cycles */
    /* FixMe - unificate with dependers/followers */
    TList<TString> conditionalJumpTargets;
    TString jumpCondition;
    ui32 jumpTimes;

    static const ui32 JUMP_UNLIMITED = ~(ui32)0;

    bool DontStop;
    bool NoResetOnTypeChange;
    /*
     * Add option "no_restart_after_loss" to target in scenario to disable its restart
     * if it was lost during worker restart or migration
     */
    bool NoRestartAfterLoss;

    inline void updateTaskResourceUsage(const TTargetTypeParameters::TId& nTask);
    void reDefineResources(const TTargetTypeParameters::TId& nTask);
    void formatRequestFromStatistics(const TTargetTypeParameters::TId& nTask);

    pid_t Run(const TTargetTypeParameters::TId& nTask);

public:
    void CheckState() const override /* override */;

    void DumpStateExtra(TPrinter& out) const override /* override */;

    bool GetDNTStatus() const;
    bool GetNoRestartAfterLoss() const;
};

