#pragma once

#include <tools/clustermaster/common/gather_subgraph.h>
#include <tools/clustermaster/common/messages.h>
#include <tools/clustermaster/common/precomputed_task_ids.h>
#include <tools/clustermaster/common/target.h>

#include <util/generic/algorithm.h>

class TMasterTarget;

class TCronState {
public:
    TCronState()
        : State(NProto::RS_INITIAL)
        , ChangedTime(TInstant::Now())
        , NumberOfCommandRetries(0)
        , Failed(false)
        , Subgraph()
        , SubgraphHosts()
    {
    }

    TCronState(NProto::ERestartState state, TInstant changedTime)
        : State(state)
        , ChangedTime(changedTime)
        , NumberOfCommandRetries(0)
        , Failed(false)
        , Subgraph()
        , SubgraphHosts()
    {
    }

    NProto::ERestartState GetState() const {
        return State;
    }

    const TInstant& GetChangedTime() const {
        return ChangedTime;
    }

    int GetNumberOfCommandRetries() const {
        return NumberOfCommandRetries;
    }

    void CommandWasRetried() {
        ++NumberOfCommandRetries;
    }

    void SetStateResetRetries(NProto::ERestartState State) {
        this->State = State;
        this->ChangedTime = TInstant::Now();
        this->NumberOfCommandRetries = 0;
    }

    bool IsFailed() const {
        return Failed;
    }

    void SetFailed(bool failed) {
        Failed = failed;
    }

    class TSubgraphTargetWithTasks {
    public:
        typedef ui32 TTaskId;
        typedef TVector<TTaskId> TTasks;

        TSubgraphTargetWithTasks(const TMasterTarget* target)
            : Target(target)
            , Tasks(new TTasks())
        {
        }

        void AddTask(TTaskId task) {
            Tasks->push_back(task);
        }

        const TMasterTarget* GetTarget() const {
            return Target;
        }

        const TTasks& GetTasks() const {
            return *Tasks;
        }

    private:
        TSubgraphTargetWithTasks();

        const TMasterTarget* Target;
        TSimpleSharedPtr<TTasks> Tasks;
    };

    class TSubgraph : TNonCopyable {
    public:
        typedef TVector<TSubgraphTargetWithTasks> TTargets;

        void AddTarget(const TSubgraphTargetWithTasks& target) {
            Targets.push_back(target);
        }

        const TTargets& GetTargets() const {
            return Targets;
        }

    private:
        TTargets Targets;
    };

    void GatherSubgraph(const TMasterTarget& target, const TString& host);

    const TSubgraph& GetSubgraph() const {
        Y_VERIFY(Subgraph.Get() != nullptr);
        return *Subgraph;
    }

    const THashSet<TString>& GetSubgraphHosts() const {
        Y_VERIFY(SubgraphHosts.Get() != nullptr);
        return *SubgraphHosts;
    }

    TVector<TString> GetSubgraphHostsSorted() const {
        Y_VERIFY(SubgraphHosts.Get() != nullptr);
        TVector<TString> result(SubgraphHosts->begin(), SubgraphHosts->end());
        Sort(result.begin(), result.end());
        return result;
    }

private:
    NProto::ERestartState State;
    TInstant ChangedTime;
    int NumberOfCommandRetries;
    bool Failed;

    TSimpleSharedPtr<TSubgraph> Subgraph;
    /**
     * Optimization - when we gather cron's subgraph we can also gather subgraph's hosts
     * and use them to send cron's commands only to several hosts - not to every host (as we do
     * in case of ordinary - initiated by user - command).
     */
    TSimpleSharedPtr<THashSet<TString> > SubgraphHosts;
};
