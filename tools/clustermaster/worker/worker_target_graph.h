#pragma once

#include "resource_manager.h"
#include "target_graph_impl.h"
#include "worker_depend.h"
#include "worker_target.h"
#include "worker_target_graph_types.h"
#include "worker_variables.h"

#include <tools/clustermaster/common/gather_subgraph.h>
#include <tools/clustermaster/proto/variables.pb.h>

#include <util/generic/hash.h>
#include <util/system/thread.h>

struct TFullStatusMessage;
struct TMultiPokeMessage;
struct TConfigMessage;
class TGraphChangeWatcher;

class TWorkerGraph: public TTargetGraphBase<TWorkerGraphTypes> {
    friend class TWorkerTarget;

public:
    class TNoConfigException {
    };
private:
    const TFsPath GetVariablesFilePath() const;
    NProto::TVariablesDump PackVariables() const;
    void SaveVariablesToYt(const NProto::TVariablesDump &protoState) const;
    void SaveVariablesToDisk(const NProto::TVariablesDump &protoState) const;
    TMaybe<NProto::TVariablesDump> LoadVariablesFromYt() const;
    TMaybe<NProto::TVariablesDump> LoadVariablesFromDisk() const;
    /*
     * CLUSTERMASTER-129
     * This source is used for migration from older CM versions.
     * Later on it should be removed.
     */
    void LoadVariablesFromDiskDeprecated();
    void LoadVariables();
    void CheckLostTasks(TLostTargetsMMap *finished);
    void ProcessFinishedLostTasks(const TLostTargetsMMap &finished);
    void RestartLostTasks(const TLostTargetsMMap &finished);

public:
    TWorkerGraph(TWorkerListManager* l, TWorkerGlobals* globals, bool forTest = false);
    void TryReadySomeTasks(TGraphChangeWatcher& watcher);
    void TryReapSomeTasks();

    void CheckExpiredTasks();
    void ProcessLostTasks(bool restartLostTasks);
    void CheckLowFreeSpace();

    void StopTasks();
    void ContinueTasks();

    // Command interface
    void Command(ui32 flags, const TTaskState& state);
    void Command(ui32 flags, const TString& target, const TTaskState& state);
    void Command(ui32 flags, const TString& target, int nTask, const TTaskState& state);
    void Command(ui32 flags, const TWorkerTarget& target, const NGatherSubgraph::TMask& input, const TTaskState& state);

    void Command2(ui32 flags, const TString& target);
    void Command2(ui32 flags, const TString& target, const TCommandMessage2::TRepeatedTasks& tasks);

    void Poke(ui32 flags, const TString& target);
    void Poke(ui32 flags, const TString& target, const TMultiPokeMessage::TRepeatedTasks& tasks);

    // Target interface
    void TaskStarted(TWorkerTarget* target, int nTask, pid_t pid);

    void LoadState();

    void ExportFullStatus(TFullStatusMessage *message) const;

    void DumpStateExtra(TPrinter&) const override;

    void CalculateTargetPriorities();

    void ImportConfig(const TConfigMessage *message, bool dump = true);

    void SaveVariables() const;
    void ImportVariables(const TVariablesMessage *message);
    void ExportVariables(TVariablesMessage *message) const;

    const TWorkerVariables& GetVariables() const;

    void ProcessResourcesEvents();

    void SetAllTaskState(const TTaskState& state);

    struct TNoResourcesHint: yexception {};
    void FormatResourcesHint(const TString& target, const TString& taskNStr, IOutputStream& out) const;

    class TResourcesEventsThread: public TThread {
    private:
        volatile bool Terminate;
        TWorkerGraph* const Graph;
        static void* ThreadProc(void* param) noexcept;
    public:
        TResourcesEventsThread(TWorkerGraph* graph);
        ~TResourcesEventsThread();
    };

    const TString& GetWorkerNameCheckDefined() const {
        if (WorkerName.empty()) {
            ythrow TWithBackTrace<yexception>();
        }
        return WorkerName;
    }

    bool IsLowFreeSpaceModeOn() const { return LowFreeSpaceModeOn; }

    TAutoPtr<TNetworkAddress> GetSolverHttpAddressForTask(const TString& targetName, const TString& taskN);

    const TMutex& GetMutex() { return Mutex; }

    struct TWorkerTargetEdgePredicate {
        static bool TargetIsOk(const TWorkerTarget& target) {
            Y_UNUSED(target);
            return true;
        }

        static bool EdgeIsOk(const TWorkerDepend& edge) {
            return !(edge.GetFlags() & DF_NON_RECURSIVE);
        };

        static bool EdgeIsOkAndActive(const TWorkerDepend& edge) {
            TWorkerGraph* graph = edge.GetSource()->Type->Graph;
            return graph->GetVariables().CheckCondition(edge.GetCondition()); // TODO This is not
                    // correct. We should check condition not against variables on worker where
                    // GatherSubgraph is executed but against variables on worker where given task
                    // should be run. But to implement this we need (somehow) get all variables on
                    // each worker.
        }
    };

    struct TWorkerDepfailTargetEdgePredicate {
        static bool TargetIsOk(const TWorkerTarget& target) {
            return target.Type->IsOnThisWorker() && !target.HasCrossnodeDepends;
        }

        static bool EdgeIsOk(const TWorkerDepend& edge) {
            return edge.IsLocal();
        };

        static bool EdgeIsOkAndActive(const TWorkerDepend& edge) {
            TWorkerGraph* graph = edge.GetSource()->Type->Graph;
            return graph->GetVariables().CheckCondition(edge.GetCondition());
        }
    };

protected:
    void ApplyGlobalVariables();

    void ProcessFinishedTask(pid_t childPid, TGraphChangeWatcher &watcher);
    void ProcessStoppedTask(pid_t childPid, TGraphChangeWatcher &watcher);
    void ProcessContinuedTask(pid_t childPid, TGraphChangeWatcher &watcher);

    struct TTaskHandle {
        TString Target;
        int NTask;

        TTaskHandle(const TString& tgt, int tsk)
            : Target(tgt)
            , NTask(tsk)
        {
        }
    };

public:
    typedef THashMap<pid_t, TTaskHandle> TPidMap;

public:
    TWorkerGlobals* WorkerGlobals;

    /* skipRootTarget is used when 'depfail' is propagated inside one worker */
    void Depfail(TWorkerTarget* startTarget, const TTargetTypeParametersMap<bool>& startTargetMask,
            TGraphChangeWatcher& watcher, bool skipRootTarget);

private:
    TResourceMonitor ResourceMonitor;
    TPidMap RunningTasks;
    TWorkerVariables Variables;
    TMutex Mutex;
    TString WorkerName;
    bool LowFreeSpaceModeOn;
};
