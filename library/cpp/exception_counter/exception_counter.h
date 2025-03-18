#pragma once
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NExceptionCounter {
    TAtomicBase GetCurrentExceptionCounter();
}
