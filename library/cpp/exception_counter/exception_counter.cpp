#include "exception_counter.h"

namespace NExceptionCounter {
    static TAtomic GlobalExceptionCounter;

    TAtomicBase GetCurrentExceptionCounter() {
        return AtomicGet(GlobalExceptionCounter);
    }

    static void HitExceptionCounter(void*, void*, void(*)(void*)) noexcept {
        AtomicIncrement(GlobalExceptionCounter);
    }
}

typedef void (*cxa_throw_hook_t)(void*, void*, void(*)(void*)) noexcept;

cxa_throw_hook_t cxa_throw_hook = NExceptionCounter::HitExceptionCounter;
