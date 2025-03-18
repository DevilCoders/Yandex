#pragma once

#include <atomic>
#include <stddef.h>
#include <util/system/compiler.h>

namespace NMarket::NAllocSetup {
    void ThrowOnError(size_t allocSize);
    void SetThrowConditions(size_t currSize, size_t maxSize);
    void SetHardLimit(size_t hardLimit);
    void SetAllocationThreshold(size_t allocationThreshold);
    void SetCacheSizeToStartReclaim(size_t value);
    void SetCacheSizeToStopReclaim(size_t value);
    void SetReclaimDivisor(size_t value);
    void SetMapPopulate(bool on);
    void SetThreadLocalCacheSizeInBytes(size_t size);
    void SetPreparedMemorySizeInBytes(size_t size);
    void SetPrepareMemoryFunc(void (*func)(size_t));
    void SetHugePagesInPreparedMemory(bool);
    bool CanAlloc(size_t allocSize, size_t totalAllocSize);
    bool NeedReclaim(size_t gcSize_, size_t counter);
    size_t GetTotalAllocSize();
    size_t GetCurSize();
    size_t GetGCSize();
    bool IsTooSmallToUnmap(size_t order);

    size_t GetHardLimit();
    size_t GetAllocationThreshold();
    size_t GetReclaimDivisor();
    bool GetReclaimIsActive();
    bool GetMapPopulateFlag();
    size_t GetThreadLocalCacheSizeInBytes();
    size_t GetPreparedMemorySizeInBytes();
    bool GetHugePagesInPreparedMemory();
    void PrepareMemory(size_t size = 0);
    void SetMinimumBlockSizeAllowedToUnmap(size_t size);


    Y_COLD void SetBallocForeverAndOnly();

    extern std::atomic<bool> ballocForeverAndOnly;

    Y_FORCE_INLINE bool IsBallocForeverAndOnly() {
        return ballocForeverAndOnly.load(std::memory_order_relaxed);
    }

    bool IsEnabledByDefault();

    struct TAllocGuard {
        TAllocGuard(size_t maxSize) {
            SetThrowConditions(0, maxSize);
        }
        ~TAllocGuard() {
            SetThrowConditions(0, 0);
        }
    };
}  // namespace NMarket::NAllocSetup
