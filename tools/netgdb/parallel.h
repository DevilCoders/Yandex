#pragma once

#include <util/system/thread.h>
#include <util/system/spinlock.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

template <class T>
struct TParallelArg {
    TVector<T> Args; // need to copy
    TVector<T>* Fails;
};

template <class T, class TFunctor>
void *ParallelTh(void* arg)
{
    TParallelArg<T>* pa = (TParallelArg<T>*)arg;
    while (true) {
        T arg;
        static TAdaptiveLock lock;
        {
            TGuard l(lock);
            if (pa->Args.empty())
                break;
            arg = *pa->Args.begin();
            pa->Args.erase(pa->Args.begin());
        }
        TFunctor functor;
        if (!functor(arg))
        {
            TGuard l(lock);
            pa->Fails->push_back(arg);
        }
    }
    return nullptr;
}

template <class T, class TFunctor>
void ParallelTask(const TVector<T>& args, TVector<T>* fails, int tasksCount = 20)
{
    TParallelArg<T> pa = {args, fails};
    pa.Fails->resize(0);
    TVector<TThread*> threads;
    for (int i = 0; i < tasksCount; i++) {
        TThread* thread = new TThread(ParallelTh<T, TFunctor>, &pa);
        threads.push_back(thread);
        thread->Start();
    }

    for (int i = 0; i < threads.ysize(); ++i) {
        delete threads[i];
    }
}
