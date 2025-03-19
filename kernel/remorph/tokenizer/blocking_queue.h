#pragma once

#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/system/guard.h>
#include <util/generic/deque.h>
#include <util/stream/debug.h>

template <class TElement>
class TBlockingQueue {
private:
    typedef TDeque<TElement> TQueueType;
private:
    TMutex M;
    TCondVar CanPopCV;
    TCondVar CanPushCV;
    size_t Limit;
    bool Finished;
    TQueueType Queue;
private:
    inline bool CanPush() const {
        return Queue.size() <= Limit;
    }
    inline bool CanPop() const {
        return Queue.size() > 0 || Finished;
    }
    bool PopL(TElement& e) {
        if (Finished && Queue.empty()) {
            CanPopCV.Signal();
            return false;
        }
        e = Queue.front();
        Queue.pop_front();
        if (Finished)
            CanPopCV.Signal();
        if (CanPush())
            CanPushCV.Signal();
        return true;
    }
    void PushL(const TElement& e) {
        Y_ASSERT(!Finished);
        Queue.push_back(e);
        if (CanPop())
            CanPopCV.Signal();
    }
public:
    TBlockingQueue(size_t limit)
        : Limit(limit)
        , Finished(false)
    {
    }
    // Blocks until queue size > LM or queue is in Finished state
    // return true if element was popped and false if queue in Finished state
    bool Pop(TElement& e) {
        TGuard<TMutex> g(M);
        while (!CanPop())
            CanPopCV.WaitI(M);
        return PopL(e);
    }
    void Push(const TElement& e) {
        TGuard<TMutex> g(M);
        while (!CanPush())
            CanPushCV.WaitI(M);
        PushL(e);
    }
    void SetFinished(bool finished = true) {
        TGuard<TMutex> g(M);
        Finished = finished;
        if (finished && CanPop())
            CanPopCV.Signal();
    }

    inline bool Empty() const {
        TGuard<TMutex> g(M);
        return Queue.empty();
    }
};

