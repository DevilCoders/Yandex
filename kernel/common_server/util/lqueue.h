#pragma once

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/generic/deque.h>
#include <util/system/mutex.h>
#include <util/system/condvar.h>

template <class T>
class TLQueue {
private:
    TMutex Mutex;
    TCondVar CV;
    TDeque<T> Queue;
    ui32 MaxSize;
    ui32 TimeWaitMs;
    TAtomic SizeAtomic = 0;
public:
    TLQueue(ui32 maxSize = 0, ui32 timeWaitMs = 1000)
        : MaxSize(maxSize)
        , TimeWaitMs(timeWaitMs)
    {

    }

    bool IsEmpty() const {
        return !Size();
    }

    ui64 Size() const {
        return AtomicGet(SizeAtomic);
    }

    inline bool Get(T* value, const TDuration* customWaitDuration = nullptr) {
        TGuard<TMutex> g(Mutex);
        if (!Queue.size()) {
            CV.WaitT(Mutex, customWaitDuration ? *customWaitDuration : TDuration::MilliSeconds(TimeWaitMs));
            if (!Queue.size())
                return false;
        }
        *value = Queue.front();
        Queue.pop_front();
        AtomicDecrement(SizeAtomic);
        return true;
    }

    inline bool Put(const T& value, bool force = false) {
        TGuard<TMutex> g(Mutex);
        if ((!MaxSize || Queue.size() < MaxSize) || force) {
            AtomicIncrement(SizeAtomic);
            Queue.push_back(value);
            CV.Signal();
            return true;
        } else
            return false;
    }
};
