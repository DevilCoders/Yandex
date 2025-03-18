#pragma once

#include <library/cpp/compute_graph/state/manager.h>

#include <util/generic/fwd.h>
#include <util/generic/ptr.h>

#include <functional>

class IThreadPool;

namespace NComputeGraph {
    using TJobId = size_t;
    using TPriority = ui32;

    using TJob = std::function<void()>;

    using TJobVector = TVector<TJobId>;

    const TJobVector& Empty();

    class IJobProcessHandler {
    public:
        virtual void Handle(ui32 completed, ui32 total) const = 0;
        virtual ~IJobProcessHandler() = default;
    };

    using TJobProcessHandlerAutoPtr = TAutoPtr<IJobProcessHandler>;
    using TJobProcessHandlerHolder = THolder<IJobProcessHandler>;

    class TPercentPrintProcessHandler: public IJobProcessHandler {
    public:
        void Handle(ui32 completed, ui32 total) const override;
    };

    /** Simple tool to use calculation graph.
    Not very fast: use it for heavy jobs.
    Note: do not use with high number of jobs (approximately 64k+).
 */
    class TJobRunner {
    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        /// Uses threadQueue if not `nullptr`. Creates instance of TThreadPool instead.
        /// Use stateManager to track finished tasks (tasks with an empty id will be executed anyway)
        TJobRunner(ui32 maxParallelThreads = 0, IThreadPool* threadQueue = nullptr,
                   const TStateManagerPtr& stateManager = nullptr, bool verbose = true);
        ~TJobRunner();

        // Will fail on exception (TRUE) or wait already running threads to finish (FALSE).
        void SetTerminateOnExceptions(bool foe = true);

        void SetStateManager(const TStateManagerPtr& stateManager);

        /** @dependsOn - list of jobs' ids that should be executed before.
        Returns new job's id.
        Job ids are compact, starting from 0.
        Job can be:
            - Any class with operator() method.
            - Any class inherited from TJob.
            - Simple void() function.
            - Object's [non-]static method (syntax: std::function< void() >(&object, &TObjectClass::MethodName)).
            - And much more! Learn <functional>!
            - Some examples you can find in junk/smikler/compute_graph/main.cpp.
        The higher the priority, the faster job starts executing. Effective for low-density graphs.
        In the case of jobs with same priority - jobs with lower id (added earlier) will be started earlier.
        Enjoy!
     */
        TJobId AddJob(TJob job, const TJobVector& dependsOn = Empty(), TPriority priority = 0);

        // id - name of the task, will be printed if task started / failed / finished
        // When used with a state manager, finished tasks will be tracked between
        // different runs of the program and will not be run twice
        // An empty id means that the task will not be tracked by the state manager
        TJobId AddJob(TJob job, const TTaskId& id,
                      const TJobVector& dependsOn = Empty(), TPriority priority = 0);

        /* barrier is just an aggregate dependency
       useful for dependency graph optimization and code simplification
    */
        TJobId AddBarrier(const TJobVector& dependsOn);

        /** Runs all compute graph.
        If one of jobs failed:
            * Remembers yexception.
            * Waits for all already started jobs.
            * Terminates the calculation (no other jobs started).
            * Throws remembered yexceptions.
     */
        void Run();

        void Clear();

        void SetProcessHandler(TJobProcessHandlerAutoPtr handler);
    };

}
