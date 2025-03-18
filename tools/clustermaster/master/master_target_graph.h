#pragma once

#include "command_alias.h"
#include "master.h"
#include "master_config.h"
#include "master_depend.h"
#include "master_target.h"
#include "master_target_graph_types.h"
#include "messages.h"
#include "reload_script_state.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/async_jobs.h>
#include <tools/clustermaster/common/gather_subgraph.h>
#include <tools/clustermaster/common/lockablehandle.h>
#include <tools/clustermaster/common/state_file.h>
#include <tools/clustermaster/common/target_graph_impl.h>
#include <tools/clustermaster/common/util.h>
#include <tools/clustermaster/proto/state_row.pb.h>
#include <tools/clustermaster/proto/variables.pb.h>

#include <kernel/yt/dynamic/table.h>

#include <library/cpp/tvmauth/client/facade.h>

#include <util/generic/ptr.h>
#include <util/stream/file.h>
#include <util/ysaveload.h>

#include <utility>

class TWorkerPool;
class TMasterListManager;

struct TFullStatusMessage;
struct TSingleStatusMessage;
struct TThinStatusMessage;
struct TConfigMessage;

namespace NAsyncJobs {
struct TGenerateGraphImageJob;
}

using TTargetSubgraph = NGatherSubgraph::TResult<TMasterTarget>;
using TTargetSubgraphPtr = TAtomicSharedPtr<TTargetSubgraph>;
using TTargetSubgraphList = TVector<TTargetSubgraphPtr>;

using TTaggedSubgraphs = std::map<TString, TTargetSubgraphList>;

class TMasterGraph: public TTargetGraphBase<TMasterGraphTypes> {
public:
    TMasterGraph(TMasterListManager* l, bool forTest = false);
    ~TMasterGraph() override {}

    void ExportConfig(const TString& worker, TConfigMessage* msg) const;
    void LoadConfig(const TMasterConfigSource& configPath, IOutputStream* dumpStream);
    void SaveVariables() const;
    void ConfigureWorkers(const TLockableHandle<TWorkerPool>::TReloadScriptWriteLock& pool);

    void ProcessFullStatus(TFullStatusMessage& message, const TString& workerhost, TWorkerPool* pool);
    void ProcessSingleStatus(TSingleStatusMessage& message, const TString& workerhost, TWorkerPool* pool);
    void ProcessThinStatus(TThinStatusMessage& message, const TString& workerhost, TWorkerPool* pool);

    void PropagateThinStatus(TString const& targetName, const TTaskState& state, TWorkerPool* pool);

    void ProcessPokes(const TLockableHandle<TWorkerPool>::TWriteLock& pool);

    void SetAllTaskState(const TTaskState& state);

    static void* GraphImageThreadProc(void* graphHandle) throw ();

public:
    void ProcessCronEntries(const TLockableHandle<TWorkerPool>::TWriteLock& pool, TInstant now, bool processTriggeredCron);

    void ProcessCronEntriesPerSecond(const TLockableHandle<TWorkerPool>::TWriteLock& pool, TInstant now);
    void SetHasObservers() throw () { HasObservers = true; }

private:
    void RetryOnFailure(const TLockableHandle<TWorkerPool>::TWriteLock& pool, TInstant now, TTarget& target);
    void ProcessFailedCron(const TMasterTarget& target, const TString& workerName, TCronState& rsct, bool& stateChanged);
    void SchedulePokeUpdate(TMasterTarget* target, TWorkerPool* pool);
    void UpdateStateStats(const TTargetsList::const_iterator& target, TMasterTarget::THostId hostId,
        const TTaskState& fromState, const TTaskState& toState);
    const TFsPath GetVariablesFilePath() const;
    NProto::TVariablesDump PackOnlyVariables(const TVariablesMap& vars) const;
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
    TMaybe<NProto::TVariablesDump> LoadVariablesFromDiskDeprecated() const;
    void LoadVariables() const;

public:
    const TTaggedSubgraphs& GetTaggedSubgraphs() const;
    void AddTaggedSubgraph(const TString& tagName, const TTargetSubgraphPtr& subgraph);

    void GatherCronSubgraphs();
    const TString* GetGraphImage() const throw () { return GraphImage.Get(); }

    TInstant GetTimestamp() const throw () { return Timestamp; }

    void CmdWholePathOnWorkers(const TMasterTarget& target, const TWorker& onWorker, const THashSet<TString>& coveredHosts,
            const TLockableHandle<TWorkerPool>::TWriteLock& pool, ui32 cmd);

    void CmdWholePathOnAllWorkers(const TMasterTarget& target, const TWorker& onWorker,
            const TLockableHandle<TWorkerPool>::TWriteLock& pool, ui32 cmd);

    bool EveryTaskHasStatus(const TCronState::TSubgraph& subgraph, const TTaskState* states, size_t statesCount) const;
    bool SomeTaskHasStatus(const TCronState::TSubgraph& subgraph, const TTaskState* states, size_t statesCount) const;

    void MaintainReloadScriptState(WorkersStat workersStat);
    bool ScriptReloadInProgress() { return ReloadScriptState.IsInProgress(); }

    void UpdateTargetsStats();

    struct TMasterTargetEdgePredicate {
        static bool TargetIsOk(const TMasterTarget& target) {
            Y_UNUSED(target);
            return true;
        }

        static bool EdgeIsOk(const TMasterDepend& edge) {
            return edge.GetCondition().IsEmpty(); // All non-conditional depends // TODO Actually we
                    // should deal with conditional depends here and do this correctly (by
                    // checking condition against workers variables - which we have on mater too).
        }

        static bool EdgeIsOkAndActive(const TMasterDepend& edge) {
            Y_UNUSED(edge);
            return true;
        }
    };

    /*
     * This predicate allows conditional edges in comparison to TMasterTargetEdgePredicate.
     */
    struct TMasterTargetEdgeInclusivePredicate {
        static bool TargetIsOk(const TMasterTarget& target) {
            Y_UNUSED(target);
            return true;
        }

        static bool EdgeIsOk(const TMasterDepend& edge) {
            Y_UNUSED(edge);
            return true;
        }

        static bool EdgeIsOkAndActive(const TMasterDepend& edge) {
            Y_UNUSED(edge);
            return true;
        }
    };

    void DispatchFailureNotifications();

    void ClearPendingPokeUpdatesAndFailureNotificationsDispatch();

    void AddVariable(const TString& name, const TString& value) {
        {
            TLockableHandle<TVariablesMap>::TWriteLock map(Variables);
            map->erase(name);
            map->insert(std::make_pair(name, value));
        }
        SaveVariables();
    }

    void DeleteVariable(const TString& name) {
        {
            TLockableHandle<TVariablesMap>::TWriteLock map(Variables);
            map->erase(name);
        }
        SaveVariables();
    }

    const TLockableHandle<TVariablesMap>& GetVariables() const {
        return Variables;
    }

private:
    THolder<TConfigMessage> Config;

    THolder<TString> GraphImage;
    ui64 GraphvizCrc;

    bool HasObservers;

    TInstant Timestamp;

    TReloadScriptState ReloadScriptState;

    unsigned PerformedPokeUpdates;
    typedef TSet<TMasterTarget*> TargetsSet;
    TargetsSet PendingPokeUpdates;

    TLockableHandle<TVariablesMap> Variables;

    TargetsSet PendingFailureNotificationsDispatch; // We need to gather subgraphs of failed targets to send mails about child targets as well (if
            // they have mailto option)

    TTaggedSubgraphs TaggedSubgraphs;

    std::shared_ptr<NTvmAuth::TTvmClient> TvmClient;

    TString TelegramSecret;

    TString JNSSecret;

    friend struct NAsyncJobs::TGenerateGraphImageJob;
};

namespace NAsyncJobs {

struct TGenerateGraphImageJob: TAsyncJobs::IJob {
    TLockableHandle<TMasterGraph> Graph;

    TGenerateGraphImageJob(const TLockableHandle<TMasterGraph>& graph)
        : Graph(graph)
    {
    }

    void Process(void*) override;
};

} // NAsyncJobs
