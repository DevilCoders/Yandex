#include "precomputed_task_ids.h"

void TDependEdgesEnumeratorRealFakeStates::TStateCollection::Append(ui32 value) {
    States.push_back(value);
}

bool TDependEdgesEnumeratorRealFakeStates::TStateCollection::Has(ui32 value) const {
    return std::binary_search(States.begin(), States.end(), value);
}

TDependEdgesEnumeratorRealFakeStates::TDependEdgesEnumeratorRealFakeStates()
    : AllStatesAreReal(true)
    , RealStates()
    , StatesCount(0)
{}

bool TDependEdgesEnumeratorRealFakeStates::IsStateReal(ui32 state) const {
    return AllStatesAreReal || RealStates.Has(state);
}

bool TDependEdgesEnumeratorRealFakeStates::IsStateFake(ui32 state) const {
    return !IsStateReal(state);
}

void TDependEdgesEnumeratorRealFakeStates::RegisterStateReal(ui32 state) {
    Y_VERIFY(state == StatesCount, "Unexpected state");
    if (!AllStatesAreReal)
        RealStates.Append(state);
    StatesCount++;
}

void TDependEdgesEnumeratorRealFakeStates::RegisterStateFake(ui32 state) {
    Y_VERIFY(state == StatesCount, "Unexpected state");
    if (AllStatesAreReal) {
        AllStatesAreReal = false;
        for (ui32 i = 0; i < state; i++)
            RealStates.Append(i);
    }
    StatesCount++;
}

size_t TDependEdgesEnumeratorRealFakeStates::GetStatesCount() const {
    return StatesCount;
}

TDependEdgesEnumerator::TDependEdgesEnumerator(const TPrecomputedTasksIds& precomputedTaskIds)
    : PrecomputedTaskIds(precomputedTaskIds)
    , NAnyState(-1)
    , N(-1)
{
}

ui32 TDependEdgesEnumerator::GetN() const {
    return N;
}

ui32 TDependEdgesEnumerator::GetNAnyState() const {
    return NAnyState;
}

bool TDependEdgesEnumerator::Next() {
    do {
        NAnyState++;
    } while (NAnyState < PrecomputedTaskIds.GetEnumeratorRealFakeStates()->GetStatesCount() &&
             PrecomputedTaskIds.GetEnumeratorRealFakeStates()->IsStateFake(NAnyState));
    N++;
    return NAnyState < PrecomputedTaskIds.GetEnumeratorRealFakeStates()->GetStatesCount();
}

TTaskIdsForEnumeratorState TDependEdgesEnumerator::GetDepTaskIds() const {
    return TTaskIdsForEnumeratorState(PrecomputedTaskIds.GetDepTaskIds(), N);
}

TTaskIdsForEnumeratorState TDependEdgesEnumerator::GetMyTaskIds() const {
    return TTaskIdsForEnumeratorState(PrecomputedTaskIds.GetMyTaskIds(), N);
}
