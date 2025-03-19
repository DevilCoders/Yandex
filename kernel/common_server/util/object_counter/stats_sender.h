#pragma once

#include <atomic>

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/system/mutex.h>
#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/thread/factory.h>

namespace NCSUtil {

    class TStatsSender {
    public:
        static constexpr TDuration kSendPeriod = TDuration::Seconds(1);

    private:
        TMutex Mutex;
        THashMap<TString, TAtomicSharedPtr<TAtomic>> Counters;
        THolder<IThreadFactory::IThread> Worker;
        TAtomic Running = 1;

    public:
        TStatsSender();
        ~TStatsSender();

        template <class T>
        static TAtomicSharedPtr<TAtomic> AddCounter() {
            return AddCounterByString(CppDemangle(typeid(T).name()));
        }

        static TAtomicSharedPtr<TAtomic> AddCounterByString(const TString& key) {
            TGuard<TMutex> g(Singleton<TStatsSender>()->Mutex);
            const auto counter = MakeAtomicShared<TAtomic>(0);
            Singleton<TStatsSender>()->Counters.emplace(key, counter);
            return counter;
        }

    private:
        void SendStats();
    };

}
