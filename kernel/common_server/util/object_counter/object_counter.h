#pragma once

#include "object_counter_helper.h"

#include <library/cpp/deprecated/atomic/atomic.h>

namespace NCSUtil {

    template <class T>
    class TObjectCounter {
    public:
        TObjectCounter() {
            IncrementCounter();
        }

        TObjectCounter(const TObjectCounter& /*counter*/) {
            IncrementCounter();
        }

        ~TObjectCounter() {
            DecrementCounter();
        }

        static i64 ObjectCount() noexcept {
            return *TObjectCounterHelper<T>::GetCounter();
        }

    private:
        void IncrementCounter() {
           AtomicAdd(*TObjectCounterHelper<T>::GetCounter(), 1); 
        }

        void DecrementCounter() {
           AtomicSub(*TObjectCounterHelper<T>::GetCounter(), 1); 
        }
    };

}
