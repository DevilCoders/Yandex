#include "controller.h"

namespace NRemorphCompiler {

TController::TController(TThreadPool& queue, TCompileTask::TPtrs& tasks, IOutputStream* log)
    : Queue(queue)
    , ControlledTasks()
    , TasksToProcess(0)
    , ProcessMutex()
    , EnqueueMutex()
    , FinishCondition()
{
    for (TCompileTask::TPtrs::const_iterator iTask = tasks.begin(); iTask != tasks.end(); ++iTask) {
        ControlledTasks.push_back(TControlledTask::TPtr(new TControlledTask(*this, **iTask)));
        const TControlledTask::TPtr& controlledTask = ControlledTasks.back();
        (*iTask)->SetNotifier(controlledTask.Get());
    }

    InitDependencies(log);
}

void TController::NotifyTaskFinished(const TControlledTask& controlledTask) {
    for (TControlledTask::TWeakPtrs::const_iterator iRevDep = controlledTask.ReverseDependencies.begin(); iRevDep != controlledTask.ReverseDependencies.end(); ++iRevDep) {
        if (!controlledTask.Task.GetStatus()) {
            (*iRevDep)->Lock();
        }
        if ((*iRevDep)->Unseal()) {
            (*iRevDep)->CheckLocks();
            if (Enqueue((*iRevDep)->Task)) {
                // Ждем пока основной поток не закончит раскидывать задачи и не встанет на ожидание.
                TGuard<TMutex> signalGuard(ProcessMutex);
                FinishCondition.Signal();
            }
        }
    }
}

void TController::Process(size_t threads) {
    Reset();
    Queue.Start(threads);

    {
        TGuard<TMutex> guard(ProcessMutex);

        bool wait = true;
        for (TControlledTask::TPtrs::const_iterator iTask = ControlledTasks.begin(); iTask != ControlledTasks.end(); ++iTask) {
            if ((*iTask)->Dependencies == 0) {
                if (Enqueue((*iTask)->Task)) {
                    wait = false;
                }
            }
        }

        if (wait) {
            FinishCondition.Wait(ProcessMutex);
        }
    }

    Queue.Stop();
}

void TController::InitDependencies(IOutputStream* log) {
    typedef THashMap<TString, TControlledTask::TWeakPtr> TTasksByOutputPath;
    TTasksByOutputPath tasksByOutputPath;

    for (TControlledTask::TPtrs::const_iterator iTask = ControlledTasks.begin(); iTask != ControlledTasks.end(); ++iTask) {
        std::pair<TTasksByOutputPath::const_iterator, bool> result = tasksByOutputPath.insert(::std::make_pair((*iTask)->OutputPath(), iTask->Get()));
        if (!result.second) {
            throw yexception() << "Unit is duplicated: \"" << (*iTask)->OutputPath() << "\"";
        }
    }

    for (TControlledTask::TPtrs::iterator iTask = ControlledTasks.begin(); iTask != ControlledTasks.end(); ++iTask) {
        TControlledTask::TWeakPtr task = iTask->Get();
        const TUnitConfig::TDependencies& dependencies = task->Task.GetUnit().GetDependencies();
        for (TUnitConfig::TDependencies::const_iterator iDep = dependencies.begin(); iDep != dependencies.end(); ++iDep) {
            TTasksByOutputPath::const_iterator iDepTaskByOutputPath = tasksByOutputPath.find(*iDep);
            if (iDepTaskByOutputPath == tasksByOutputPath.end()) {
                TFsPath depPath(*iDep);
                if (!depPath.Exists() || !depPath.IsFile()) {
                    throw yexception() << "Dependency not found: " << *iDep;
                }
                continue;
            }
            TControlledTask::TWeakPtr depTask = iDepTaskByOutputPath->second;
            if (task == depTask) {
                throw yexception() << "Reflexive dependency detected: \"" << task->OutputPath() << "\"";
            }
            if (task->FindReverseDependency(depTask)) {
                throw yexception() << "Cyclic dependency detected: \"" << task->OutputPath() << "\" <--> \"" << depTask->OutputPath() << "\"";
            }
            if (log) {
                *log << "Dependency: " << task->OutputPath() << Endl;
                *log << "  on: " << depTask->OutputPath() << Endl;
            }
            depTask->ReverseDependencies.push_back(task);
            ++task->Dependencies;
        }
    }
}

void TController::Reset() {
    TasksToProcess = ControlledTasks.size();

    for (TControlledTask::TPtrs::const_iterator iTask = ControlledTasks.begin(); iTask != ControlledTasks.end(); ++iTask) {
        TControlledTask& task = **iTask;
        task.Task.ResetError();
        task.Seal(task.Dependencies);
        task.Locks = 0;
    }
}

bool TController::Enqueue(TCompileTask& task) {
    {
        TGuard<TMutex> enqueueQuard(EnqueueMutex);
        Queue.SafeAdd(&task);
    }
    return AtomicDecrement(TasksToProcess) == 0;
}

} // NRemorphCompiler
