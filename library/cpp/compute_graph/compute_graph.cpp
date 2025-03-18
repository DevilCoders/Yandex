#include "compute_graph.h"

#include <util/thread/pool.h>
#include <util/system/mutex.h>
#include <util/system/guard.h>
#include <util/system/pipe.h>
#include <util/string/cast.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/datetime/base.h>

#include <queue>

namespace NComputeGraph {
    void TPercentPrintProcessHandler::Handle(ui32 completed, ui32 total) const {
        Cerr << (100 * completed / total) << "% status" << Endl;
    }

    class TSemaphore {
    public:
        inline TSemaphore(size_t initialCount = 0) {
            TPipeHandle::Pipe(R_, W_);

            for (size_t i = 0; i < initialCount; ++i) {
                Release();
            }
        }

        inline void Acquire() {
            char ch;
            R_.Read(&ch, 1);
        }

        inline void Release() {
            W_.Write("", 1);
        }

    private:
        TPipeHandle R_;
        TPipeHandle W_;
    };

    struct TJobIdAndPriority {
        TJobId JobId;
        TPriority Priority;

        TJobIdAndPriority(TJobId jobId, TPriority priority)
            : JobId(jobId)
            , Priority(priority)
        {
        }
    };

    TString GetIdString(const TTaskId& tId, const TJobId& jId) {
        if (!tId.Empty()) {
            return tId.Str() + " (" + ToString(jId) + ")";
        } else {
            return ToString(jId);
        }
    }

    struct TJobContext {
        TJob Job;
        TJobId JobId;
        TTaskId TaskId;
        TPriority Priority;
        size_t DependenciesCompleted;
        size_t DependenciesRequired;
        bool Finished;
        bool Failed;
        TString ExceptionString;

        TJobContext(TJob job, TJobId jobId, TTaskId TaskId, TPriority priority,
                    size_t dependenciesRequired = 0)
            : Job(job)
            , JobId(jobId)
            , TaskId(TaskId)
            , Priority(priority)
            , DependenciesCompleted(0)
            , DependenciesRequired(dependenciesRequired)
            , Finished(false)
            , Failed(false)
        {
        }

        TJobIdAndPriority GetJobIdAndPriority() const {
            return TJobIdAndPriority(JobId, Priority);
        }
    };

    bool operator<(const TJobIdAndPriority& x, const TJobIdAndPriority& y) {
        if (x.Priority != y.Priority) {
            return x.Priority < y.Priority;
        } else {
            return x.JobId > y.JobId;
        }
    }

    using TJobQueue = std::priority_queue<TJobIdAndPriority>;
    using TJobDependencies = THashMap<TJobId, TJobVector>;

    struct TJobContextInfos {
        TVector<TJobContext> Contexts;
        TJobDependencies Dependencies;
        TJobQueue ReadyToRun;
        TJobVector FinishedWithError;
        mutable ui32 RunningNow;
        mutable ui32 Finished;
        bool TerminateOnException = false;
        bool Verbose = true;

        void Clear() {
            Contexts.clear();
            Dependencies.clear();
            for (; !ReadyToRun.empty(); ReadyToRun.pop()) {
            }
            FinishedWithError.clear();
            RunningNow = 0;
            Finished = 0;
        }
    };

    static TMutex CerrLock;

    TString PrintCurrentTime() {
        return TInstant::Now().ToStringUpToSeconds();
    }

    class TJobThread: public IObjectInQueue {
    private:
        TJobContext* Context;
        TJobContextInfos* ContextsInfos;
        TMutex* ContextInfosMutex;
        TSemaphore* MasterAwaker;
        IJobProcessHandler* ProcessHandler;
        TMutex* ProcessHandlerMutex;

        TStateManager* StateManager;

        void Process(void* /*ThreadSpecificResource*/) override {
            const TJobId jobId = Context->JobId;

            bool hasException = false;

            try {
                bool started = true;
                if (!Context->TaskId.Empty()) {
                    if (StateManager) {
                        started = StateManager->StartTask(Context->TaskId);
                    }

                    if (started && ContextsInfos->Verbose) {
                        TGuard<TMutex> g(CerrLock);
                        Cerr << PrintCurrentTime() << " Task " << Context->TaskId.Str() << " started" << Endl;
                    }
                }
                if (started) {
                    Context->Job();
                    if (!Context->TaskId.Empty()) {
                        if (StateManager) {
                            StateManager->FinishTask(Context->TaskId);
                        }
                        if (ContextsInfos->Verbose) {
                            TGuard<TMutex> g(CerrLock);
                            Cerr << PrintCurrentTime() << " Task " << Context->TaskId.Str() << " finished" << Endl;
                        }
                    }
                    Context->Finished = true;
                } else {
                    while (true) {
                        if (StateManager->HasFinished(Context->TaskId)) {
                            if (ContextsInfos->Verbose) {
                                TGuard<TMutex> g(CerrLock);
                                Cerr << PrintCurrentTime() << " Task " << Context->TaskId.Str() <<
                                    " skipped (finished)" << Endl;
                            }
                            Context->Finished = true;
                            break;
                        } else if (StateManager->HasFailed(Context->TaskId)) {
                            ythrow yexception() << "Parent task failed: " << Context->TaskId.Str();
                        }

                        // There is the same task already running, let's wait
                        // If you think that this is inefficient (it is) - don't use
                        // the same TaskID in multiple threads or implement a better solution
                        Sleep(TDuration::Seconds(1));
                    }
                }
            } catch (...) {
                if (!Context->TaskId.Empty() && StateManager) {
                    StateManager->FailTask(Context->TaskId);
                }
                hasException = true;
                Context->Failed = true;
                TStringStream ss;
                ss << "Exception in job #" << jobId;
                if (!Context->TaskId.Empty()) {
                    ss << " " << Context->TaskId.Str();
                }
                ss << ": " << CurrentExceptionMessage();
                Context->ExceptionString = ss.Str();
                if (ContextsInfos->TerminateOnException) {
                    std::terminate();
                }
            }

            {
                TGuard<TMutex> guard(*ContextInfosMutex);

                if (!hasException) {
                    const TJobVector& dependencies = ContextsInfos->Dependencies[jobId];

                    for (size_t i = 0; i < dependencies.size(); ++i) {
                        const TJobId contextId = dependencies[i];

                        TJobContext& context = ContextsInfos->Contexts[contextId];

                        context.DependenciesCompleted++;

                        if (context.DependenciesCompleted == context.DependenciesRequired) {
                            ContextsInfos->ReadyToRun.push(context.GetJobIdAndPriority());
                        }
                    }
                } else {
                    ContextsInfos->FinishedWithError.push_back(jobId);

                    Cerr << "ERROR: " << Context->ExceptionString << Endl;
                }

                ContextsInfos->RunningNow--;
                ContextsInfos->Finished++;

                {
                    TGuard<TMutex> phmGuard(ProcessHandlerMutex);

                    if (ProcessHandler != nullptr) {
                        ProcessHandler->Handle(ContextsInfos->Finished, ContextsInfos->Contexts.size());
                    }
                }

                MasterAwaker->Release();
            }
        }

    public:
        TJobThread(TJobContext* context,
                   TJobContextInfos* contextsInfos,
                   TMutex* contextInfosMutex,
                   TSemaphore* masterAwaker,
                   IJobProcessHandler* processHandler,
                   TMutex* processHandlerMutex,
                   TStateManager* stateManager)
            : Context(context)
            , ContextsInfos(contextsInfos)
            , ContextInfosMutex(contextInfosMutex)
            , MasterAwaker(masterAwaker)
            , ProcessHandler(processHandler)
            , ProcessHandlerMutex(processHandlerMutex)
            , StateManager(stateManager)
        {
        }
    };

    using TJobThreadPtr = TSimpleSharedPtr<TJobThread>;

    class TJobRunner::TImpl: public TJobContextInfos {
    private:
        size_t TotalJobs;
        ui32 MaxThreads;

        IThreadPool* PredefinedMtpQueue;
        TAtomicSharedPtr<TThreadPool> MtpQueue;

        TStateManagerPtr StateManager;

        TJobProcessHandlerHolder JobProcessHandlerHolder;
        TMutex JobProcessHandlerMutex;
        TMutex JobFinishMutex;

    public:
        TImpl(ui32 maxThreads = 0, IThreadPool* threadQueue = nullptr,
              const TStateManagerPtr& stateManager = nullptr, bool verbose = true)
            : TotalJobs(0)
            , MaxThreads(maxThreads)
            , PredefinedMtpQueue(threadQueue)
            , StateManager(stateManager)
        {
            Verbose = verbose;
            TJobContextInfos::Clear();
        }

        void SetTerminateOnExceptions(bool foe) {
            TerminateOnException = foe;
        }

        void SetStateManager(const TStateManagerPtr& m) {
            StateManager = m;
        }

        void SetProcessHandler(TJobProcessHandlerAutoPtr handler) {
            TGuard<TMutex> guard(JobProcessHandlerMutex);
            JobProcessHandlerHolder.Reset(handler.Release());
        }

        TJobId AddJob(TJob job, const TTaskId& id, const TJobVector& dependencies, TPriority priority) {
            TGuard<TMutex> g(JobFinishMutex);

            const TJobId jobId = TotalJobs;

            if (jobId == 0 && dependencies.size() != 0) {
                ythrow yexception() << "Job #0 must have no dependencies!";
            }

            size_t numDependencies = 0;
            bool failed = false;
            for (size_t i = 0; i < dependencies.size(); ++i) {
                const TJobId dependsOnId = dependencies[i];

                if (dependsOnId >= jobId) {
                    ythrow yexception() << "Job may depends only on jobs with lower id!";
                }

                if (Contexts[dependsOnId].Failed) {
                    failed = true;
                    Cerr << GetIdString(id, jobId) << " failed due to the dependency: " <<
                        GetIdString(Contexts[dependsOnId].TaskId, dependsOnId) << Endl;
                } else if (!Contexts[dependsOnId].Finished) {
                    Dependencies[dependsOnId].push_back(jobId);
                    ++numDependencies;
                }
            }

            Contexts.push_back(TJobContext(job, jobId, id, priority, numDependencies));

            if (!numDependencies && !failed) {
                ReadyToRun.push(Contexts.back().GetJobIdAndPriority());
            }

            TotalJobs++;

            return jobId;
        }

        void Run() {
            if (TotalJobs == 0) {
                return; // nothing to calculate.
            }

            // To avoid reallocations while computing results.
            FinishedWithError.reserve(TotalJobs);

            TSemaphore masterCond;

            TVector<TJobThreadPtr> threads;

            IThreadPool* mtpQueuePtr;

            if (PredefinedMtpQueue == nullptr) {
                MtpQueue = new TThreadPool();
                mtpQueuePtr = MtpQueue.Get();
            } else {
                mtpQueuePtr = PredefinedMtpQueue;
            }

            size_t lastRunJobs = Finished;
            const ui32 realMaxThreads = (MaxThreads != 0) ? MaxThreads : (TotalJobs - lastRunJobs);

            IThreadPool& mtpQueue = *mtpQueuePtr;

            mtpQueue.Start(realMaxThreads, Max<ui32>());
            RunningNow = 0;

            TJobId jobsStarted = lastRunJobs;
            while (true) {
                if (!FinishedWithError.empty()) {
                    break; // Some errors occured.
                }

                {
                    TGuard<TMutex> guard(JobFinishMutex);

                    // Condition (RunningNow < realMaxThreads) helps not to push in queue tasks, in case of tasks with greater priority comes before.
                    while (!ReadyToRun.empty() && RunningNow < realMaxThreads) {
                        const TJobId jobId = ReadyToRun.top().JobId;

                        threads.push_back(
                            new TJobThread(
                                &Contexts[jobId],
                                static_cast<TJobContextInfos*>(this),
                                &JobFinishMutex,
                                &masterCond,
                                JobProcessHandlerHolder.Get(),
                                &JobProcessHandlerMutex,
                                StateManager.Get()));

                        mtpQueue.SafeAdd(threads.back().Get());
                        ++RunningNow;

                        ReadyToRun.pop();
                        ++jobsStarted;
                    }
                }

                if (jobsStarted == TotalJobs) {
                    break; // all jobs started
                }

                masterCond.Acquire();
            }

            mtpQueue.Stop(); // all jobs competed

            if (!FinishedWithError.empty()) {
                TString errMessage = "";
                for (size_t i = lastRunJobs; i < FinishedWithError.size(); ++i) {
                    const TJobId failedJobId = FinishedWithError[i];
                    errMessage += "\n\t";
                    errMessage += Contexts[failedJobId].ExceptionString;
                }
                ythrow yexception() << "EXECUTION IS TERMINATED. One or more jobs finished with errors!" << errMessage;
            }
        }

        void Clear() {
            TJobContextInfos::Clear();
            TotalJobs = 0;
            if (StateManager) {
                StateManager->Clear();
            }
        }
    };

    TJobRunner::TJobRunner(ui32 maxThreads, IThreadPool* threadQueue,
                           const TStateManagerPtr& stateManager, bool verbose) {
        Impl.Reset(new TImpl(maxThreads, threadQueue, stateManager, verbose));
    }

    TJobRunner::~TJobRunner() = default;

    TJobId TJobRunner::AddJob(TJob job, const TJobVector& dependencies, TPriority priority) {
        return Impl->AddJob(job, "", dependencies, priority);
    }

    TJobId TJobRunner::AddJob(TJob job, const TTaskId& id, const TJobVector& dependencies, TPriority priority) {
        return Impl->AddJob(job, id, dependencies, priority);
    }

    void EmptyJob() {
    }

    TJobId TJobRunner::AddBarrier(const TJobVector& dependsOn) {
        return Impl->AddJob(&EmptyJob, "", dependsOn, 0);
    }

    void TJobRunner::Run() {
        Impl->Run();
    }

    void TJobRunner::Clear() {
        Impl->Clear();
    }

    void TJobRunner::SetProcessHandler(TJobProcessHandlerAutoPtr handler) {
        Impl->SetProcessHandler(handler);
    }

    void TJobRunner::SetTerminateOnExceptions(bool foe) {
        Impl->SetTerminateOnExceptions(foe);
    }

    void TJobRunner::SetStateManager(const TStateManagerPtr& stateManager) {
        Impl->SetStateManager(stateManager);
    }

    const TJobVector& Empty() {
        return Default<TJobVector>();
    }

}
