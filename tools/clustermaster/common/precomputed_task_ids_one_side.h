#pragma once

#include <util/generic/vector.h>
#include <util/system/defaults.h>

class TTaskIdsForEnumeratorState;

class TPrecomputedTaskIdsForOneSide {
public:
    typedef ui32 TTaskId;
    typedef TVector<TTaskId> TTaskIdsList;

private:
    typedef TVector<TTaskIdsList*> TListsOfLists;

    /**
     * Usually there are _a lot_ of targets where each dependency (one dependency == one TDependEdgesEnumerator state) has only one
     * task on some/both sides. If we put tasks for all dependencies to single list we save a lot of memory.
     */
    TTaskIdsList SingleTasks;
    TListsOfLists ListOfLists;
    bool SingleTasksMode;

    void ConvertToListOfListsMode();

public:
    TPrecomputedTaskIdsForOneSide();
    ~TPrecomputedTaskIdsForOneSide();

    ui32 Length(ui32 enumeratorState ) const;
    void AddSingleTaskId(TTaskId taskId);
    void AddTaskIdsList(const TTaskIdsList& list);
    ui64 UsedMemory() const;
    TTaskId At(ui32 enumeratorState, ui32 taskIndex) const;
    void ShrinkToFit();

    class TIterator {
    public:
        TIterator(const TPrecomputedTaskIdsForOneSide* ids, ui32 enumeratorState, ui32 taskIndex)
            : Ids(ids)
            , EnumeratorState(enumeratorState)
            , TaskIndex(taskIndex)
        {}

        TTaskId operator*() const;
        TIterator& operator++();

        bool operator==(const TIterator&) const;
        bool operator!=(const TIterator&) const;

    private:
        const TPrecomputedTaskIdsForOneSide* Ids;
        const ui32 EnumeratorState;
        ui32 TaskIndex;
    };

    TTaskIdsForEnumeratorState TasksAt(ui32 enumeratorState) const;
};

class TTaskIdsForEnumeratorState {
private:
    const TPrecomputedTaskIdsForOneSide* Ids;
    ui32 EnumeratorState;

public:
    TTaskIdsForEnumeratorState(const TPrecomputedTaskIdsForOneSide* ids, ui32 enumeratorState);

    TPrecomputedTaskIdsForOneSide::TIterator Begin() const;
    TPrecomputedTaskIdsForOneSide::TIterator End() const;
    ui32 Length() const;
    TVector<TPrecomputedTaskIdsForOneSide::TTaskId> ToVector() const;
};
