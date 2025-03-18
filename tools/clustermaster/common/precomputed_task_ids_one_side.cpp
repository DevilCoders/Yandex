#include "precomputed_task_ids_one_side.h"

#include <util/system/yassert.h>

#include <algorithm>

TPrecomputedTaskIdsForOneSide::TPrecomputedTaskIdsForOneSide()
    : SingleTasksMode(true)
{}

TPrecomputedTaskIdsForOneSide::~TPrecomputedTaskIdsForOneSide() {
    for (TListsOfLists::iterator i = ListOfLists.begin(); i != ListOfLists.end(); i++)
        delete *i;
}

ui32 TPrecomputedTaskIdsForOneSide::Length(ui32 enumeratorState) const {
    if (SingleTasksMode)
        return 1;
    else
        return ListOfLists[enumeratorState]->size();
}

void TPrecomputedTaskIdsForOneSide::ConvertToListOfListsMode() {
    for (TTaskIdsList::iterator i = SingleTasks.begin(); i != SingleTasks.end(); i++) {
        ListOfLists.push_back(new TTaskIdsList);
        ListOfLists.back()->push_back(*i);
    }
    SingleTasks.clear();
    SingleTasksMode = false;
}

void TPrecomputedTaskIdsForOneSide::AddSingleTaskId(TTaskId taskId) {
    if (SingleTasksMode) {
        SingleTasks.push_back(taskId);
    } else {
        ListOfLists.push_back(new TTaskIdsList);
        ListOfLists.back()->push_back(taskId);
    }
}

void TPrecomputedTaskIdsForOneSide::AddTaskIdsList(const TTaskIdsList& list) {
    if (SingleTasksMode)
        ConvertToListOfListsMode();
    ListOfLists.push_back(new TTaskIdsList(list));
}

ui64 TPrecomputedTaskIdsForOneSide::UsedMemory() const {
    if (SingleTasksMode) {
        return sizeof(TTaskIdsList) + SingleTasks.capacity() * sizeof(ui32);
    } else {
        ui64 res = sizeof(TListsOfLists) + ListOfLists.capacity() * sizeof(TTaskIdsList*);
        for (size_t i = 0; i < ListOfLists.size(); i++) {
            res += sizeof(TTaskIdsList) + ListOfLists[i]->capacity() * sizeof(ui32);
        }
        return res;
    }
}

TPrecomputedTaskIdsForOneSide::TTaskId TPrecomputedTaskIdsForOneSide::At(ui32 enumeratorState, ui32 taskIndex) const {
    if (SingleTasksMode) {
        Y_VERIFY(taskIndex == 0);
        return SingleTasks[enumeratorState];
    } else {
        const TTaskIdsList& taskIds = *ListOfLists[enumeratorState];
        return taskIds[taskIndex];
    }
}

void TPrecomputedTaskIdsForOneSide::ShrinkToFit() {
    if (SingleTasksMode) {
        TTaskIdsList(SingleTasks).swap(SingleTasks);
    } else {
        TListsOfLists(ListOfLists).swap(ListOfLists);
    }
}

TPrecomputedTaskIdsForOneSide::TTaskId TPrecomputedTaskIdsForOneSide::TIterator::operator*() const {
    return Ids->At(EnumeratorState, TaskIndex);
}

TPrecomputedTaskIdsForOneSide::TIterator& TPrecomputedTaskIdsForOneSide::TIterator::operator++() {
    ++TaskIndex;
    return *this;
}

bool TPrecomputedTaskIdsForOneSide::TIterator::operator==(const TPrecomputedTaskIdsForOneSide::TIterator& other) const {
    return Ids == other.Ids && EnumeratorState == other.EnumeratorState && TaskIndex == other.TaskIndex;
}

bool TPrecomputedTaskIdsForOneSide::TIterator::operator!=(const TPrecomputedTaskIdsForOneSide::TIterator& other) const {
    return !((*this) == other);
}

TTaskIdsForEnumeratorState TPrecomputedTaskIdsForOneSide::TasksAt(ui32 enumeratorState) const {
    return TTaskIdsForEnumeratorState(this, enumeratorState);
}

TTaskIdsForEnumeratorState::TTaskIdsForEnumeratorState(const TPrecomputedTaskIdsForOneSide* ids, ui32 enumeratorState)
    : Ids(ids)
    , EnumeratorState(enumeratorState)
{
}

TPrecomputedTaskIdsForOneSide::TIterator TTaskIdsForEnumeratorState::Begin() const {
    return TPrecomputedTaskIdsForOneSide::TIterator(Ids, EnumeratorState, 0);
}

TPrecomputedTaskIdsForOneSide::TIterator TTaskIdsForEnumeratorState::End() const {
    return TPrecomputedTaskIdsForOneSide::TIterator(Ids, EnumeratorState, Ids->Length(EnumeratorState));
}

ui32 TTaskIdsForEnumeratorState::Length() const {
    return Ids->Length(EnumeratorState);
}

TVector<TPrecomputedTaskIdsForOneSide::TTaskId> TTaskIdsForEnumeratorState::ToVector() const {
    TVector<TPrecomputedTaskIdsForOneSide::TTaskId> result;
    for (TPrecomputedTaskIdsForOneSide::TIterator i = Begin(); i != End(); ++i) {
        result.push_back(*i);
    }
    return result;
}
