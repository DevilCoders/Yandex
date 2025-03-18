#pragma once

#include <util/generic/ptr.h>

/**
 * Recursive exclusive lock with fair wait queue.
 */
class TRecursiveFairLock {
public:
    TRecursiveFairLock();
    ~TRecursiveFairLock();

    void Acquire() noexcept;
    bool TryAcquire() noexcept;
    void Release() noexcept;

    bool IsFree() noexcept;
    bool IsOwnedByMe() noexcept;

private:
    class TImpl;
    THolder<TImpl> Impl;
};
