#include "semaphore.h"

#include "graph_change_watcher.h"
#include "worker_target.h"

TSemaphore::TSemaphore(TWorkerTarget* target)
    : BusyMask(target->GetTasks().Size(), true)
    , Used(0)
    , Width(target->GetTasks().Size()), Limit(1)
{
    Targets.push_back(target);
}

void TSemaphore::SetLimit(unsigned int limit) {
    Limit = limit;
}

void TSemaphore::AddTarget(TWorkerTarget* target) {
    if (Targets.empty())
        Width = target->GetTasks().Size();
    else if (Width != target->GetTasks().Size())
        ythrow yexception() << "mismatch for semaphore target";

    if (Find(Targets.begin(), Targets.end(), target) == Targets.end())
        Targets.push_back(target);
}

void TSemaphore::Update() {
    if (BusyMask.size() != Width)
        ythrow yexception() << "semaphore width mismatch";

    TVector<bool> AllSuccessMask(BusyMask.size(), true);
    TVector<bool> AnyDirtyMask(BusyMask.size(), false);

    for (TVector<TWorkerTarget*>::iterator target = Targets.begin(); target != Targets.end(); ++target) {
        if ((*target)->GetTasks().Size() != Width)
            continue; // XXX: throw?

        for (TWorkerTarget::TTaskList::TConstEnumerator task = (*target)->GetTasks().Enumerator(); task.Next(); ) {
            if (task->GetState() != TS_SUCCESS)
                AllSuccessMask[task.CurrentN().GetN()] = false;

            // READY should not be counted as free as semaphore is checked before communism
            if (task->GetState() != TS_IDLE &&
                    task->GetState() != TS_PENDING &&
                    task->GetState() != TS_SKIPPED &&
                    task->GetState() != TS_FAILED &&
                    task->GetState() != TS_DEPFAILED)
                AnyDirtyMask[task.CurrentN().GetN()] = true;
        }
    }

    Used = 0;
    for (unsigned int i = 0; i < BusyMask.size(); ++i) {
        BusyMask[i] = AnyDirtyMask[i] && !AllSuccessMask[i];
        if (BusyMask[i])
            Used++;
    }
}

bool TSemaphore::TryRun(const TTargetTypeParameters::TId& nTask) {
    // Group is already runnung -> ok to run more in this group
    if (BusyMask[nTask.GetN()])
        return true;

    // Group is not running and limit is reached -> can't run anything
    if (Used >= Limit)
        return false;

    // Run new group
    Used++;
    BusyMask[nTask.GetN()] = true;
    return true;
}

void TSemaphore::TryReadySomeTasks(TGraphChangeWatcher& watcher) {
    for (TVector<TWorkerTarget*>::iterator i = Targets.begin(); i != Targets.end(); ++i)
        (*i)->TryReadySomeTasks(watcher);
}

bool TSemaphore::IsLast(TWorkerTarget* target) const {
    return !Targets.empty() && Targets.back() == target;
}
