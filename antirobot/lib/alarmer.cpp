#include "alarmer.h"

#include <util/generic/vector.h>


namespace NAntiRobot {

    TAlarmer::TTaskId TAlarmer::Add(TDuration period, int count, bool rightNow, TCallback cb) {
        TAtomicSharedPtr<TAlarmTask> task(new TAlarmTask);
        task->Callback = std::move(cb);
        task->Count = count;
        task->Period = period;
        task->NextTime = TInstant::Now() + (rightNow ? TDuration::MicroSeconds(0) : period);

        {
            TGuard<TMutex> guard(TaskListMutex);
            TaskMap[++LastId] = std::move(task);
            return LastId;
        }
    }

    void TAlarmer::Remove(TTaskId id) {
        if (id == INVALID_ID) {
            return;
        }

        with_lock (TaskListMutex) {
            TaskMap.erase(id);
        }

        // Make sure that we don't return until TAlarmer::Run finishes its last execution of the
        // removed task except for cases when TAlarmer::Remove is called by this task.
        if (CurrentThreadId() != Id()) {
            TReadGuard removeGuard(RemoveMutex);
        }
    }

    void* TAlarmer::ThreadProc(void* _this) {
        TThread::SetCurrentThreadName("TAlarmer");
        static_cast<TAlarmer*>(_this)->Run();
        return nullptr;
    }

    void TAlarmer::ExecuteTask(const TAlarmer::TAlarmTask& task) {
        try {
            if (task.Callback) {
                task.Callback();
            }
        } catch (...) {}
    }

    void TAlarmer::Run() {
        while (!QuitEvent.WaitT(TickPeriod)) {
            TInstant now = TInstant::Now();

            TWriteGuard removeGuard(RemoveMutex);

            TVector<TAtomicSharedPtr<const TAlarmTask>> executeTasks;
            TVector<TTaskId> removeTasks;

            with_lock (TaskListMutex) {
                for (const auto& [taskId, task] : TaskMap) {
                    if (now >= task->NextTime) {
                        executeTasks.push_back(task);
                        task->NextTime = now + task->Period;

                        if (task->Count > 0 && --task->Count == 0) {
                            removeTasks.push_back(taskId);
                        }
                    }
                }

                for (const auto& taskId : removeTasks) {
                    TaskMap.erase(taskId);
                }
            }

            for (const auto& task : executeTasks) {
                ExecuteTask(*task);
            }
        }
    }
}
