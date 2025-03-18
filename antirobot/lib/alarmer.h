#pragma once

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/system/event.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>
#include <util/system/thread.h>

#include <functional>

namespace NAntiRobot {
    class TAlarmer : private TThread, public TNonCopyable {
    public:
        using TCallback = std::function<void()>;
        using TTaskId = size_t;
        static constexpr TTaskId INVALID_ID = 0;

    private:
        struct TAlarmTask {
            TCallback Callback;
            size_t Count;
            TDuration Period;
            TInstant NextTime;
        };

        THashMap<size_t, TAtomicSharedPtr<TAlarmTask>> TaskMap;
        size_t LastId;

        TMutex TaskListMutex;
        TSystemEvent QuitEvent;
        TRWMutex RemoveMutex;
        TDuration TickPeriod;

        static void* ThreadProc(void* _this);
        void ExecuteTask(const TAlarmTask& task);

    public:
        explicit TAlarmer(TDuration tickPeriod = TDuration::MilliSeconds(200))
            : TThread(&ThreadProc, this)
            , LastId(0)
            , TickPeriod(tickPeriod)
        {
            Start();
        }

        ~TAlarmer() {
            QuitEvent.Signal();
            Join();
        }

        // count: 0 - forever, >0 specified times
        TTaskId Add(TDuration period, int count, bool rightNow, TCallback cb);
        void Remove(TTaskId id);

        void Run();

        using TThread::Join;
    };

    class TAlarmTaskHolder : public TNonCopyable {
    public:
        TAlarmTaskHolder() = default;

        TAlarmTaskHolder(TAlarmer& alarmer, TAlarmer::TTaskId taskId) {
            Reset(alarmer, taskId);
        }

        TAlarmTaskHolder(
            TAlarmer& alarmer,
            TDuration period, int count, bool rightNow, TAlarmer::TCallback cb
        ) {
            Reset(alarmer, period, count, rightNow, std::move(cb));
        }

        void Reset(TAlarmer& alarmer, TAlarmer::TTaskId taskId) {
            AlarmerAndTask.ConstructInPlace(alarmer, taskId);
        }

        void Reset(
            TAlarmer& alarmer,
            TDuration period, int count, bool rightNow, TAlarmer::TCallback cb
        ) {
            AlarmerAndTask.ConstructInPlace(
                alarmer,
                alarmer.Add(period, count, rightNow, std::move(cb))
            );
        }

    private:
        struct TData {
            TAlarmer& Alarmer;
            TAlarmer::TTaskId TaskId;

            explicit TData(TAlarmer& alarmer, TAlarmer::TTaskId taskId)
                : Alarmer(alarmer)
                , TaskId(taskId)
            {}

            ~TData() {
                Alarmer.Remove(TaskId);
            }
        };

        TMaybe<TData> AlarmerAndTask;
    };
}
