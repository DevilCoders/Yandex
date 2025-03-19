#pragma once

#include "stats_sender.h"

#include <util/generic/ptr.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NCSUtil {

    template <class T>
    class TObjectCounterHelper {
    public:
        TAtomicSharedPtr<TAtomic> Counter;

    public:
        TObjectCounterHelper() {
            Counter = TStatsSender::AddCounter<T>();
        }

        static TAtomicSharedPtr<TAtomic> GetCounter() noexcept {
            const auto counter = Singleton<TObjectCounterHelper<T>>()->Counter;
            Y_VERIFY(counter);
            return counter;
        }        
    };

}
