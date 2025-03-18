#pragma once

#include "precomputed_task_ids.h"
#include "target_type_parameters_map.h"

#include <util/generic/hash.h>
#include <util/generic/list.h>

namespace NGatherSubgraph {

typedef TTargetTypeParametersMap<bool> TMask;

enum EMode {
    M_DEPFAIL,
    M_COMMAND_RECURSIVE_UP,
    M_COMMAND_RECURSIVE_DOWN
};

class TResultForTarget {
public:
    enum ESkippedState { SS_UNKNOWN, SS_SKIPPED, SS_NOT_SKIPPED };

    TResultForTarget(const TTargetTypeParameters* params)
        : Mask(params)
        , Skipped(SS_UNKNOWN)
    {
    }

    TResultForTarget(const TMask& mask)
        : Mask(mask)
        , Skipped(SS_UNKNOWN)
    {
    }

    bool IsNotSkipped() const {
        Y_VERIFY(Skipped != SS_UNKNOWN);
        return Skipped == SS_NOT_SKIPPED;
    }

    bool IsSkipped() const {
        Y_VERIFY(Skipped != SS_UNKNOWN);
        return Skipped == SS_SKIPPED;
    }

    TMask Mask;
    ESkippedState Skipped;
};

template <typename TTarget>
class TResult : TNonCopyable {
public:
    typedef THashMap<const TTarget*, TResultForTarget*> TResultByTarget;
    typedef TList<const TTarget*> TTargetList;

    TResult()
        : TopoTargets()
        , ResultByTarget()
    {}

    ~TResult() {
        for (typename TResultByTarget::iterator i = ResultByTarget.begin(); i != ResultByTarget.end(); i++) {
            delete i->second;
        }
    }

    TResultForTarget* InsertTargetResult(const TTarget* target, const TTargetTypeParameters* params) {
        return ResultByTarget.insert(std::make_pair(target, new TResultForTarget(params))).first->second;
    }

    TResultForTarget* InsertTargetResult(const TTarget* target, const TMask& mask) {
        return ResultByTarget.insert(std::make_pair(target, new TResultForTarget(mask))).first->second;
    }

    const TResultByTarget& GetResultByTarget() const {
        return ResultByTarget;
    }

    void SetTopoSortedTargets(const TTargetList& topoTargets) {
        TopoTargets = topoTargets;
    }

    const TTargetList& GetTopoSortedTargets() const {
        return TopoTargets;
    }

private:
    TTargetList TopoTargets;
    TResultByTarget ResultByTarget;
};

inline bool UseDependsByMode(EMode mode) {
    switch (mode) {
    case NGatherSubgraph::M_DEPFAIL:
        return false;
    case NGatherSubgraph::M_COMMAND_RECURSIVE_DOWN:
        return false;
    case NGatherSubgraph::M_COMMAND_RECURSIVE_UP:
        return true;
    default:
        Y_FAIL("Wrong EGatherSubgraphMode");
    }
}

template <typename PTypes, typename PParamsByType, typename PTargetEdgePredicate>
void GatherSubgraph(const typename PTypes::TTarget& startTarget, const TMask& startMask, EMode mode, TResult<typename PTypes::TTarget>* result) {
    bool useDepends = UseDependsByMode(mode);

    typedef typename PTypes::TTarget TTarget;

    typedef TList<const TTarget*> TTopoSubgraph;
    TTopoSubgraph topoSubgraph;

    typename TTarget::TTraversalGuard guard;
    startTarget.template TopoSortedSubgraph<PTargetEdgePredicate>(&topoSubgraph, useDepends, guard);

    result->SetTopoSortedTargets(topoSubgraph);

    if (topoSubgraph.empty()) { // could be empty if all targets in subgraph are not ok
        return;
    }

    topoSubgraph.pop_back(); // we don't need root - as it is startTarget
    TResultForTarget* startTargetResult = result->InsertTargetResult(&startTarget, startMask);
    startTargetResult->Skipped = TResultForTarget::SS_NOT_SKIPPED;

    while (!topoSubgraph.empty()) {
        TTarget* target = const_cast<TTarget*>(topoSubgraph.back());

        TResultForTarget* resultForTarget = result->InsertTargetResult(target, &PParamsByType::GetParams(*target->Type));

        // we are 'looking back' - so use followers if useDepends == true and vise versa
        typename TTarget::TDependsList backEdges = useDepends ? target->Followers : target->Depends;

        for (typename TTarget::TDependsList::const_iterator backEdge = backEdges.begin(); backEdge != backEdges.end(); ++backEdge) {
            const TTarget* backTarget = backEdge->GetTarget();

            typename TResult<TTarget>::TResultByTarget::const_iterator backTargetResult = result->GetResultByTarget().find(backTarget);
            if (backTargetResult == result->GetResultByTarget().end()) {
                continue;
            }

            TMask* backTargetMask = &backTargetResult->second->Mask;

            IPrecomputedTaskIdsInitializer* precomputed =
                    PParamsByType::template GetPrecomputedTaskIdsMaybe<PTypes>(*backEdge).Get();
            // precomputed->Initialize();

            bool currentEdgeIsActive = PTargetEdgePredicate::EdgeIsOkAndActive(*backEdge);

            TDependEdgesEnumerator en(precomputed->GetIds());

            for (; en.Next(); ) {
                bool dependActive = false;

                TTaskIdsForEnumeratorState realDepTasks = useDepends ? en.GetMyTaskIds() : en.GetDepTaskIds();
                TTaskIdsForEnumeratorState realMyTasks = useDepends ? en.GetDepTaskIds() : en.GetMyTaskIds();

                if (mode == M_DEPFAIL) {
                    dependActive = false;
                    for (TPrecomputedTaskIdsForOneSide::TIterator depTaskIndex = realDepTasks.Begin();
                            depTaskIndex != realDepTasks.End(); ++depTaskIndex) {
                        if (backTargetMask->At(*depTaskIndex) == true) {
                            dependActive = true;
                            break;
                        }
                    }
                } else { // M_COMMAND_RECURSIVE_UP, M_COMMAND_RECURSIVE_DOWN
                    dependActive = true;
                    for (TPrecomputedTaskIdsForOneSide::TIterator depTaskIndex = realDepTasks.Begin();
                            depTaskIndex != realDepTasks.End(); ++depTaskIndex) {
                        if (backTargetMask->At(*depTaskIndex) == false) {
                            dependActive = false;
                            break;
                        }
                    }
                }

                if (dependActive) {
                    for (TPrecomputedTaskIdsForOneSide::TIterator myTaskIndex = realMyTasks.Begin();
                            myTaskIndex != realMyTasks.End(); ++myTaskIndex) {
                        resultForTarget->Mask.At(*myTaskIndex) = true;
                    }

                    if (backTargetResult->second->IsSkipped() || !currentEdgeIsActive) {
                        if (resultForTarget->Skipped == TResultForTarget::SS_UNKNOWN) {
                            resultForTarget->Skipped = TResultForTarget::SS_SKIPPED;
                        }
                    } else {
                        resultForTarget->Skipped = TResultForTarget::SS_NOT_SKIPPED;
                    }
                }
            }
        }

        topoSubgraph.pop_back();
    }
}

}

