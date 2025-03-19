#pragma once

#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>

#include <algorithm>

template<typename iter>
class SortCtx
{
public:
        iter first;
        iter last;
        int depth;
        SortCtx<iter>* Next;

public:
    size_t size() {
        return last - first;
    }
};

template<typename iter>
class TSortThreadList;

template<class iter>
void* SortThread(void* arg);

template<typename iter>
class TSortThread {
public:
    SortCtx<iter>* Ctx;
    TSortThread<iter>* Next;
    TSortThreadList<iter>* ThreadList;
    TSystemEvent Ev;
    THolder<TThread> Thread;

public:
    TSortThread<iter>(TSortThreadList<iter>* list)
        : Ctx(nullptr)
        , Next(nullptr)
        , ThreadList(list)
        , Ev(TSystemEvent::rAuto)
    {
    }
};

template<typename iter>
class TSortThreadList {
public:
    TSortThread<iter>* ThreadList;
    SortCtx<iter>* CtxQueue;
    ui32 Running;
    struct timeval PrevTm;
    ui64 TotalTime;
    ui64 RunTime;
    TMutex Mutex;
    TSystemEvent Ev;

public:
    TSortThreadList<iter>()
        : ThreadList(nullptr)
        , CtxQueue(nullptr)
        , Running(0)
        , TotalTime(0)
        , RunTime(0)
        , Ev(TSystemEvent::rAuto)
    {
    }

    void CountTotalTime() {
        struct timeval tm;
        gettimeofday(&tm, nullptr);
        if (Running) {
            ui64 runTime = (tm.tv_sec * 1000000ULL + tm.tv_usec - (PrevTm.tv_sec * 1000000ULL + PrevTm.tv_usec));
            TotalTime += Running * runTime;
            RunTime += runTime;
        }
        PrevTm = tm;
    }

    void RunTask(SortCtx<iter>* ctx) {
        Mutex.Acquire();
        if (ThreadList) {
            CountTotalTime();
            Running++;
            TSortThread<iter>* thread1 = ThreadList;
            ThreadList = thread1->Next;
            thread1->Ctx = ctx;
            thread1->Ev.Signal();
        }
        else {
            ctx->Next = CtxQueue;
            CtxQueue = ctx;
        }
        Mutex.Release();
    }

    SortCtx<iter>* PopCtx() {
        SortCtx<iter>* ctx = nullptr;
        Mutex.Acquire();
        if (CtxQueue) {
            ctx = CtxQueue;
            CtxQueue = ctx->Next;
        }
        Mutex.Release();
        return ctx;
    }

    void InsertThread(TSortThread<iter>& thread) {
        Mutex.Acquire();
        CountTotalTime();
        Running--;
        thread.Next = ThreadList;
        ThreadList = &thread;
        Mutex.Release();
        if (Running == 0)
            Ev.Signal();
    }

    void InitThreads(int count) {
        for (int i = 0; i < count; i++) {
            TSortThread<iter>* thread = new TSortThread<iter>(this);
            thread->Next = ThreadList;
            ThreadList = thread;
            thread->Thread.Reset(new TThread(::SortThread<iter>, thread));
            thread->Thread->Start();
        }
    }

    void StopThreads() {
        while (ThreadList) {
            TSortThread<iter>* thread = ThreadList;
            ThreadList = thread->Next;
            thread->Ctx = nullptr;
            thread->Ev.Signal();
            thread->Thread->Join();
            thread->Thread.Destroy();
        }
    }
};

template<typename iter>
void RunTask(TSortThread<iter>& thread, SortCtx<iter>* ctx) {
    thread.ThreadList->RunTask(ctx);
}

template<typename iter>
void RunTask(TSortThread<iter>& thread, iter first, iter last, int depth) {
    SortCtx<iter>* ctx = new SortCtx<iter>;
    ctx->first = first;
    ctx->last = last;
    ctx->depth = depth;
    RunTask<iter>(thread, ctx);
}

template<typename iter>
void SortThread(TSortThread<iter>& thread) {
    while (true) {
        thread.Ev.Wait();
        if (thread.Ctx == nullptr)
            return;
        iter first = thread.Ctx->first;
        iter last = thread.Ctx->last;
        int depth = thread.Ctx->depth;
        delete thread.Ctx;
        while (true) {
            if ((last - first < 0x10000) || (depth > 100)) {
//                fprintf(stderr, "sorting %016lx %016lx\n", (ui64)first, (ui64)last);
                std::sort(first, last);
                SortCtx<iter>* ctx = thread.ThreadList->PopCtx();
                if (!ctx)
                    break;
                first = ctx->first;
                last = ctx->last;
                depth = ctx->depth;
//                fprintf(stderr, "popped %016lx %016lx\n", (ui64)first, (ui64)last);
                delete ctx;
            }
            else {
                iter median = first + (last - first) / 2;
                std::nth_element(first, median, last);
                RunTask(thread, median, last, depth - 1);
                last = median;
                depth--;
            }
        }
        thread.ThreadList->InsertThread(thread);
    }
}


template<class iter>
void* SortThread(void* arg) {
    TSortThread<iter>* thread = (TSortThread<iter>*)arg;
    SortThread(*thread);
    return nullptr;
}
