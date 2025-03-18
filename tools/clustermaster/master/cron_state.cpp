#include "cron_state.h"

#include "master_target_graph.h"

struct TLocalspaceShiftRecord {
    TLocalspaceShiftRecord(const TString& worker, size_t shift)
        : Worker(worker)
        , Shift(shift) {}

    TString Worker;
    size_t Shift;

    bool operator<(const TLocalspaceShiftRecord& rhs) const {
        return Shift < rhs.Shift;
    }
};

typedef TVector<TLocalspaceShiftRecord> TSortedLocalspaceShifts;

void TCronState::GatherSubgraph(const TMasterTarget& target, const TString& host) {
    using namespace NGatherSubgraph;

    Subgraph = new TSubgraph();
    SubgraphHosts = new THashSet<TString>();

    TMask mask(&target.Type->GetParameters());

    for (TMasterTarget::TConstTaskIterator i = target.TaskIteratorForHost(host); i.Next(); )
        mask.At(i.GetN().GetN()) = true;

    typedef TResult<TMasterTarget> TResult;

    TResult tempSubgraph;

    NGatherSubgraph::GatherSubgraph<TMasterGraphTypes, TParamsByTypeOrdinary, TMasterGraph::TMasterTargetEdgePredicate>
            (target, mask, M_COMMAND_RECURSIVE_UP, &tempSubgraph);

    typedef TResult::TResultByTarget TResultByTarget;
    typedef TResult::TTargetList TTargetList;

    const TResultByTarget& resultByTarget = tempSubgraph.GetResultByTarget();
    const TTargetList& topoTargets = tempSubgraph.GetTopoSortedTargets();

    // Iterating through the temp result in topological order
    for (TTargetList::const_reverse_iterator it = topoTargets.rbegin(); it != topoTargets.rend(); it++) {
        TResultByTarget::const_iterator i = resultByTarget.find(*it);
        const TMasterTarget& t = *i->first;

        TSubgraphTargetWithTasks subgraphTarget(&t);

        // Gather & sort localspace shifts for workers (need shifts to quickly find worker by task number)
        const TMasterTargetType::TLocalspaceShifts& unsortedShifts = t.Type->GetLocalspaceShifts();
        TSortedLocalspaceShifts shifts;
        for (TMasterTargetType::TLocalspaceShifts::const_iterator us = unsortedShifts.begin(); us != unsortedShifts.end(); ++us)
            shifts.push_back(TLocalspaceShiftRecord(us->first, us->second));
        Sort(shifts.begin(), shifts.end());

        size_t shiftId = 0;
        for (ui32 task = 0; task < i->second->Mask.Size(); task++) {
            if (i->second->Mask.At(task) == true && i->second->IsNotSkipped()) {
                subgraphTarget.AddTask(task);

                // Because localspace shifts are sorted, we only need to move the pointer a little (instead of starting from the beginning)
                while (shiftId < shifts.size() - 1 && shifts[shiftId + 1].Shift <= task)
                    shiftId++;

                const TString& targetWorker = shifts[shiftId].Worker;

                SubgraphHosts->insert(targetWorker);
            }
        }

        Subgraph->AddTarget(subgraphTarget);
    }
}
