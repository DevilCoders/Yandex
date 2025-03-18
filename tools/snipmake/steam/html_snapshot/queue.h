#pragma once

#include <util/system/mutex.h>
#include <util/system/guard.h>
#include <util/system/condvar.h>
#include <util/generic/deque.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <utility>

namespace NSteam
{

template<class T> class TCondVarQueue
{
private:
    TDeque<T> Queue;
    TMutex Lock;
    TCondVar Cond;

public:
    virtual void Schedule(T t)
    {
        {
            TGuard<TMutex> g(Lock);
            Queue.push_back(t);
        }
        Cond.Signal();
    }

    // Does not care about spurious wakeups
    std::pair<bool, T> Next(TDuration timeout)
    {
        std::pair<bool, T> result;
        result.first = false;

        TInstant deadline;

        bool doWait = timeout != TDuration::Zero();
        if (doWait) {
            deadline = timeout.ToDeadLine();
        }
        else {
            deadline = TInstant::Now();
        }

        TGuard<TMutex> g(Lock);

        if (Queue.empty()) {
            if (doWait) {
                result.first = Cond.WaitD(Lock, deadline);
            }
            else {
                result.first = false;
            }
        }
        else {
            result.first = true;
        }

        if (!result.first) {
            return result;
        }

        result.second = Queue.front();
        Queue.pop_front();
        return result;
    }
};

}
