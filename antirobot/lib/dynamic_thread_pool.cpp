#include "dynamic_thread_pool.h"

#include <util/generic/map.h>
#include <util/generic/stack.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>
#include <util/system/thread.h>

const size_t TDynamicThreadPool::TParams::INFINITE = Max<size_t>() / 2; // divided by 2 for avoid integer overflow INFINITE + INFINITE
thread_local TDynamicThreadPool::TThread* TDynamicThreadPool::TThread::CurrentPtr = nullptr;

size_t TDynamicThreadPool::TParams::ParseValue(const TString& valueStr) {
    static const TString INFINITE_STR = "infinite";
    return (valueStr == INFINITE_STR) ? INFINITE : FromString<size_t>(valueStr);
}

class TDynamicThreadPool::TImpl {
public:
    TImpl(const TParams& params)
    : Params(params)
    {
        if (Params.IncrementFree <= 0)
            ythrow yexception() << "TDynamicThreadPool: IncrementFree must be >= 1";
        if (Params.MinFree > Params.MaxFree)
            ythrow yexception() << "TDynamicThreadPool: must be Params.MinFree(" << Params.MinFree << ") <= Params.MaxFree(" << Params.MaxFree << ")";
        if (Params.MaxFree > Params.MaxTotal)
            ythrow yexception() << "TDynamicThreadPool: must be Params.MaxFree(" << Params.MaxFree << ") <= Params.MaxTotal(" << Params.MaxTotal << ")";

        Zero(Stat);

        const size_t initialThreadsNum = Min((Params.MinFree + 4 * Params.IncrementFree), Params.MaxFree); // empirical
        CreateThreads(initialThreadsNum);
    }

    ~TImpl();

    void Stop();

    IThreadFactory::IThread* DoCreate();

    void MinimizeFreeThreadsTo(size_t freeThreadsNum);
    void PrintStatistics(NAntiRobot::TStatsWriter& out);
    void PrintStatistics(TStatsOutput& out);
private:
    class TPoolThread;
    class TPoolThreadWrapper;
    class TPoolThreadStopGuardOps;
    typedef TSimpleSharedPtr<TPoolThread, TPoolThreadStopGuardOps> TPoolThreadStopGuardRef;
private:
    template <typename F>
    void WriteStats(F write);

    void PushFreeThread(TPoolThread* freeThread);
    size_t CreateIfNeededFreeThreads(); // must be executed under lock
    void CreateThreads(size_t num);
private:
    TParams Params;
    TMutex ThreadsStoreMutex;
    TStack<TPoolThreadStopGuardRef> FreeThreads;
    typedef TMap<TPoolThread*, TPoolThreadStopGuardRef> TBusyThreads;
    TBusyThreads BusyThreads;
    struct TStat {
        size_t GotReady;
        size_t GotStrong;
        size_t Created;
        size_t EasyCall;
        size_t Returned;
    } Stat;
    TRWMutex JoinMutex;
};

// Self guided class
// Объект этого класса может быть удален только им самим, см. ThreadProc().
class TDynamicThreadPool::TImpl::TPoolThread : public TThread {
public:
    TPoolThread(TImpl* parent)
    : Parent(parent)
    , Thr_(new ::TThread(ThreadProc, this))
    , Job(nullptr)
    , NeedDie(false)
    {
        Thr_->Start();
    }

    ~TPoolThread() {
        Thr_->Detach(); // avoid deadlock for self joining
    }

    void DoRun(IThreadAble* func) {
        {
            TGuard<TMutex> lock(GlobalMutex);
            Y_ASSERT(Job == nullptr);
            Job = func;
        }
        GlobalCondVar.Signal();
    }

    void DoJoin() noexcept {
        TGuard<TMutex> lock(GlobalMutex);
        while (Job != nullptr) {
            GlobalCondVar.WaitI(GlobalMutex);
        }
    }

    void DoOrphanAndTellStop() {
        {
            TGuard<TMutex> lock(GlobalMutex);
            NeedDie = true;
        }
        GlobalCondVar.Signal();
    }
private:
    static void* ThreadProc(void* threadObject) {
        THolder<TPoolThread> threadObjectHolder(static_cast<TPoolThread*>(threadObject));
        return threadObjectHolder->ThreadProc();
    }

    void* ThreadProc() {
        TReadGuard joinMutexGuard(Parent->JoinMutex);
        CurrentPtr = this;

        while (true) {
            {
                TGuard<TMutex> lock(GlobalMutex);
                while (Job == nullptr && !NeedDie) {
                    GlobalCondVar.WaitI(GlobalMutex);
                }
                if (NeedDie) {
                    break;
                }
            }
            Job->Execute();
            {
                TGuard<TMutex> lock(GlobalMutex);
                Job = nullptr;
                if (!NeedDie) {
                    Parent->PushFreeThread(this);
                }
            }
            GlobalCondVar.Signal();
        }

        for (auto& dtor : Destructors) {
            dtor();
        }

        CurrentPtr = nullptr;

        return nullptr;
    }
private:
    TImpl* Parent;
    THolder<::TThread> Thr_;
    TMutex GlobalMutex;
    TCondVar GlobalCondVar;
    IThreadAble* Job;
    bool NeedDie;
};

// do stop thread, but don't kill
class TDynamicThreadPool::TImpl::TPoolThreadStopGuardOps {
public:
    static void Destroy(TPoolThread* thread) {
        if (thread)
            thread->DoOrphanAndTellStop();
    }
};

// interface for actions, but not own TPoolThread
class TDynamicThreadPool::TImpl::TPoolThreadWrapper: public IThreadFactory::IThread {
public:
    TPoolThreadWrapper(TPoolThread* poolThread)
    : PoolThread(poolThread)
    {
    }

    ~TPoolThreadWrapper() override
    {
    }
private:
    void DoRun(IThreadAble* func) override {
        PoolThread->DoRun(func);
    }

    void DoJoin() noexcept override {
        PoolThread->DoJoin();
    }
private:
    TPoolThread* PoolThread;
};

TDynamicThreadPool::TDynamicThreadPool(const TParams& params)
: Impl(new TImpl(params))
{
}

IThreadFactory::IThread* TDynamicThreadPool::DoCreate() {
    Y_ASSERT(Impl.Get() != nullptr);
    return Impl->DoCreate();
}

TDynamicThreadPool::~TDynamicThreadPool() {
}

void TDynamicThreadPool::Stop() {
    Impl->Stop();
}

IThreadFactory::IThread* TDynamicThreadPool::TImpl::DoCreate() {
    TPoolThreadStopGuardRef threadGuard;
    {
        TGuard<TMutex> lock(ThreadsStoreMutex);
        if (FreeThreads.empty()) {
            CreateIfNeededFreeThreads();
        }else {
            Stat.GotReady++;
        }
        if (!FreeThreads.empty()) {
            threadGuard = FreeThreads.top();
            FreeThreads.pop();
            BusyThreads.insert(std::make_pair(threadGuard.Get(), threadGuard));
            if (!CreateIfNeededFreeThreads())
                Stat.EasyCall++;
        }
    }
    if (!threadGuard) {
        threadGuard.Reset(new TPoolThread(this));
        {
            TGuard<TMutex> lock(ThreadsStoreMutex);
            BusyThreads.insert(std::make_pair(threadGuard.Get(), threadGuard));
            Stat.GotStrong++;
        }
    }
    // what do after BusyThreads.insert() if we got exception?
    return new TPoolThreadWrapper(threadGuard.Get());
}

void TDynamicThreadPool::PrintStatistics(NAntiRobot::TStatsWriter& out) {
    Impl->PrintStatistics(out);
}

void TDynamicThreadPool::PrintStatistics(TStatsOutput& out) {
    Impl->PrintStatistics(out);
}

TDynamicThreadPool::TImpl::~TImpl() {
    Stop();
}

void TDynamicThreadPool::TImpl::Stop() {
    MinimizeFreeThreadsTo(0);

    {
        TGuard<TMutex> lock(ThreadsStoreMutex);
        BusyThreads.clear();
    }

    JoinMutex.AcquireWrite();
    JoinMutex.ReleaseWrite();
}

void TDynamicThreadPool::TImpl::MinimizeFreeThreadsTo(size_t freeThreadsNum) {
    TGuard<TMutex> lock(ThreadsStoreMutex);
    while (FreeThreads.size() > freeThreadsNum) {
        FreeThreads.pop();
    }
}

template <typename F>
void TDynamicThreadPool::TImpl::WriteStats(F write) {
    TGuard<TMutex> lock(ThreadsStoreMutex);
    write("dtp_free_threads", FreeThreads.size());
    write("dtp_busy_threads", BusyThreads.size());
    write("dtp_killed_threads", Stat.Created - FreeThreads.size() - BusyThreads.size());
    write("dtp_created", Stat.Created);
    write("dtp_returned", Stat.Returned);
    write("dtp_got_ready", Stat.GotReady);
    write("dtp_got_strong", Stat.GotStrong);
    write("dtp_easy_call", Stat.EasyCall);
}

void TDynamicThreadPool::TImpl::PushFreeThread(TPoolThread* freeThread) {
    TGuard<TMutex> lock(ThreadsStoreMutex);
    TBusyThreads::iterator it = BusyThreads.find(freeThread);
    Y_ASSERT(it != BusyThreads.end());
    TPoolThreadStopGuardRef threadGuard = it->second;
    BusyThreads.erase(it);
    Stat.Returned++;
    if (FreeThreads.size() < Params.MaxFree) {
        FreeThreads.push(threadGuard);
    }
}

void TDynamicThreadPool::TImpl::CreateThreads(size_t num) {
    for (size_t i = 0; i < num; ++i) {
        FreeThreads.push(new TPoolThread(this));
        Stat.Created++;
    }
}

// must be executed under lock
size_t TDynamicThreadPool::TImpl::CreateIfNeededFreeThreads() {
    if (FreeThreads.size() < Params.MinFree && (FreeThreads.size() + BusyThreads.size() < Params.MaxTotal)) {
        CreateThreads(Params.IncrementFree);
        return Params.IncrementFree;
    } else {
        return 0;
    }
}

void TDynamicThreadPool::TImpl::PrintStatistics(NAntiRobot::TStatsWriter& out) {
    WriteStats([&out] (TStringBuf key, size_t value) {
        out.WriteScalar(key, value);
    });
}

void TDynamicThreadPool::TImpl::PrintStatistics(TStatsOutput& out) {
    WriteStats([&out] (const TString& key, size_t value) {
        out.AddScalar(key, value);
    });
}
