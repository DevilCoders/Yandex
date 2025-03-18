#pragma once

#include "compile_task.h"
#include "config.h"

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>

namespace NRemorphCompiler {

class TController {
private:
    typedef THashMultiMap<TCompileTask*, TCompileTask*> TReverseDependencies;

    struct TControlledTask: public TCompileTask::INotifier, public TSimpleRefCount<TControlledTask> {
        typedef TIntrusivePtr<TControlledTask> TPtr;
        typedef TControlledTask* TWeakPtr;
        typedef TVector<TPtr> TPtrs;
        typedef TVector<TWeakPtr> TWeakPtrs;

        TController& Controller;
        TCompileTask& Task;
        TWeakPtrs ReverseDependencies;
        size_t Dependencies;

        TAtomic Seals;
        TAtomic Locks;

        TControlledTask(TController& controller, TCompileTask& task)
            : Controller(controller)
            , Task(task)
            , ReverseDependencies()
            , Dependencies(0)
            , Seals(0)
            , Locks(0)
        {
        }

        virtual ~TControlledTask()
        {
        }

        void Notify() override {
            Controller.NotifyTaskFinished(*this);
        }

        inline void Seal(TAtomicBase seals) {
            return AtomicSet(Seals, seals);
        }

        inline bool Unseal() {
            return AtomicDecrement(Seals) == 0;
        }

        inline void Lock() {
            AtomicIncrement(Locks);
        }

        inline void CheckLocks() const {
            if (Locks == 0) {
                return;
            }
            TStringStream error;
            error << "skipped because of " << Locks << " failed dependencies";
            Task.SetError(error.Str());
        }

        inline const TString& OutputPath() const {
            return Task.GetUnit().Output;
        }

        bool FindReverseDependency(TControlledTask::TWeakPtr revDepTask) {
            for (TWeakPtrs::const_iterator iTask = ReverseDependencies.begin(); iTask != ReverseDependencies.end(); ++iTask) {
                if ((*iTask == revDepTask) || (*iTask)->FindReverseDependency(revDepTask)) {
                    return true;
                }
            }
            return false;
        }
    };

private:
    TThreadPool& Queue;

    TControlledTask::TPtrs ControlledTasks;
    TAtomic TasksToProcess;
    TMutex ProcessMutex;
    TMutex EnqueueMutex;
    TCondVar FinishCondition;

public:
    TController(TThreadPool& queue, TCompileTask::TPtrs& tasks, IOutputStream* log = nullptr);

    virtual void NotifyTaskFinished(const TControlledTask& controlledTask);
    void Process(size_t threads);

private:
    void InitDependencies(IOutputStream* log);
    void Reset();
    bool Enqueue(TCompileTask& task);
};

} // NRemorphCompiler
