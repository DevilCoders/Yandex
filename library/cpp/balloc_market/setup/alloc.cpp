#include <new>
#include <stdio.h>
#include <stdlib.h>

#include "alloc.h"
#include "enable.h"
#include <util/system/platform.h>
#include <library/cpp/malloc/api/malloc.h>
#include <library/cpp/balloc_market/lib/balloc_defs.h>

#include <atomic>

namespace NMarket::NAllocSetup {
    std::atomic<size_t> hardLimit = {size_t(-1)};
    std::atomic<size_t> allocationThreshold = {size_t(-1)};
    std::atomic<size_t> threadLocalCacheSizeInBytes = {65536};
    std::atomic<size_t> preparedMemorySizeInBytes = {0};
    std::atomic<void (*)(size_t)> prepareMemoryFunc = {nullptr};
    std::atomic<bool> mapPopulateIsOn = {false};
    std::atomic<bool> hugePagesInPreparedMemory = {false};
    std::atomic<bool> ballocForeverAndOnly = {false};
    std::atomic<size_t> minimumBlockOrderAllowedToUnmap = {0};
    std::atomic<size_t> cacheSizeToStartReclaim = {size_t(-1)};
    std::atomic<size_t> cacheSizeToStopReclaim = {0};
    std::atomic<size_t> reclaimDivisor = {100};
    std::atomic<bool> reclaimIsActive = {false};

    struct TThrowInfo {
        std::atomic<size_t> CurrSize;
        std::atomic<size_t> MaxSize;
    };
#if defined(_unix_) && !defined(_darwin_)
    thread_local TThrowInfo info;
    void ThrowOnError(size_t allocSize) {
        size_t currSize_ =
            info.CurrSize.fetch_add(allocSize, std::memory_order_relaxed);
        size_t maxSize_ = info.MaxSize.load(std::memory_order_relaxed);
        if (maxSize_ && maxSize_ < currSize_) {
#ifndef NDEBUG
            __builtin_trap();
#endif
            info.CurrSize.store(0, std::memory_order_relaxed);
            throw std::bad_alloc();
        }
    }
    void SetThrowConditions(size_t currSize, size_t maxSize) {
        info.CurrSize.store(currSize, std::memory_order_relaxed);
        info.MaxSize.store(maxSize, std::memory_order_relaxed);
    }
#else  // _unix_ && ! _darwin_
    void ThrowOnError(size_t /*allocSize*/) {
    }
    void SetThrowConditions(size_t /*currSize*/, size_t /*maxSize*/) {
    }
#endif // _unix_ && ! _darwin_

    void SetHardLimit(size_t hardLimit_) {
        hardLimit.store(hardLimit_, std::memory_order_relaxed);
    }

    void SetAllocationThreshold(size_t allocationThreshold_) {
        allocationThreshold.store(allocationThreshold_,
            std::memory_order_relaxed);
    }

    void SetCacheSizeToStartReclaim(size_t value) {
        cacheSizeToStartReclaim.store(value, std::memory_order_relaxed);
    }

    void SetCacheSizeToStopReclaim(size_t value) {
        cacheSizeToStopReclaim.store(value, std::memory_order_relaxed);
    }

    void SetReclaimDivisor(size_t value) {
        reclaimDivisor.store(value, std::memory_order_relaxed);
    }

    void SetMapPopulate(bool on) {
        mapPopulateIsOn.store(on, std::memory_order_release);
    }

    void SetThreadLocalCacheSizeInBytes(size_t size) {
        threadLocalCacheSizeInBytes.store(size, std::memory_order_release);
    }

    void SetPreparedMemorySizeInBytes(size_t size) {
        preparedMemorySizeInBytes.store(size, std::memory_order_release);
    }

    void SetPrepareMemoryFunc(void (*func)(size_t)) {
        prepareMemoryFunc.store(func, std::memory_order_relaxed);
    }

    void SetHugePagesInPreparedMemory(bool on) {
        hugePagesInPreparedMemory.store(on, std::memory_order_release);
    }

    bool GetHugePagesInPreparedMemory() {
        return hugePagesInPreparedMemory.load(std::memory_order_acquire);
    }

    size_t GetHardLimit() {
        return hardLimit.load(std::memory_order_relaxed);
    }

    size_t GetAllocationThreshold() {
        return allocationThreshold.load(std::memory_order_relaxed);
    }

    size_t GetReclaimDivisor() {
        return reclaimDivisor.load(std::memory_order_relaxed);
    }

    bool GetReclaimIsActive() {
        return reclaimIsActive.load(std::memory_order_relaxed);
    }

    std::atomic<size_t> allocSize;
    std::atomic<size_t> totalAllocSize;
    std::atomic<size_t> gcSize;

    size_t GetTotalAllocSize() {
        return totalAllocSize.load(std::memory_order_relaxed);
    }

    size_t GetCurSize() {
        return allocSize.load(std::memory_order_relaxed);
    }

    size_t GetGCSize() {
        return gcSize.load(std::memory_order_relaxed);
    }

    bool CanAlloc(size_t allocSize_, size_t totalAllocSize_) {
        allocSize.store(allocSize_, std::memory_order_relaxed);
        totalAllocSize.store(totalAllocSize_, std::memory_order_relaxed);
        size_t allocationThreshold_ =
            allocationThreshold.load(std::memory_order_relaxed);
        size_t hardLimit_ = hardLimit.load(std::memory_order_relaxed);
        return allocSize_ < hardLimit_ ||
            totalAllocSize_ < allocationThreshold_;
    }

    bool NeedReclaim(size_t gcSize_, size_t counter) {
        gcSize.store(gcSize_, std::memory_order_relaxed);
        if (reclaimIsActive.load(std::memory_order_acquire)) {
            if (gcSize_ <= cacheSizeToStopReclaim.load(std::memory_order_relaxed)) {
                reclaimIsActive.store(false, std::memory_order_release);
                return false;
            }
        } else {
            if (gcSize_ < cacheSizeToStartReclaim.load(std::memory_order_relaxed)) {
                return false;
            }
            reclaimIsActive.store(true, std::memory_order_release);
        }
        return counter > reclaimDivisor.load(std::memory_order_relaxed);
    }

    bool IsEnabledByDefault() {
        return EnableByDefault;
    }

    bool GetMapPopulateFlag() {
        return mapPopulateIsOn.load(std::memory_order_acquire);
    }

    size_t GetThreadLocalCacheSizeInBytes() {
        return threadLocalCacheSizeInBytes.load(std::memory_order_acquire);
    }

    size_t GetPreparedMemorySizeInBytes() {
        return preparedMemorySizeInBytes.load(std::memory_order_acquire);
    }

    void PrepareMemory(size_t size) {
        void (*func)(size_t) =
            prepareMemoryFunc.load(std::memory_order_relaxed);
        if (func)
            return func(size);
    }

    Y_COLD void SetBallocForeverAndOnly() {
        if (!IsEnabledByDefault())
            NMalloc::AbortFromCorruptedAllocator();
        ballocForeverAndOnly.store(true, std::memory_order_seq_cst);
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    bool IsTooSmallToUnmap(size_t order) {
        size_t limit =
            minimumBlockOrderAllowedToUnmap.load(std::memory_order_relaxed);
        return limit > order;
    }

    void SetMinimumBlockSizeAllowedToUnmap(size_t size) {
        size = NBalloc::AlignUp(size, NBalloc::SYSTEM_PAGE_SIZE) /
            NBalloc::SYSTEM_PAGE_SIZE;
        minimumBlockOrderAllowedToUnmap.store(size, std::memory_order_relaxed);
    }

}  // namespace NMarket::NAllocSetup
