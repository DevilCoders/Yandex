#pragma once

/* ABOUT STORE AND LOAD FENCES
 * Application threads may exchange pointers to allocated objects without
 * explicit synchronization (via relaxed stores and loads). Without
 * the synchronization Free routine may read INVALID or STALE ancilliary data.
 * To perform the synchronization a store fence is required right before
 * returning allocated object to application code and load fence is required
 * right before freeing an allocated object. It is effectively achieves
 * acquire-release semantic on an arbitrary variable which is used to exchange
 * a pointer to an allocated object.
 * The store and the load fences are effectively noop on intel (and compatible)
 * platforms.
*/

#include <util/system/defaults.h>

#include <sys/mman.h>

#if defined(_linux_)
#    ifndef MADV_DONTDUMP
#        define MADV_DONTDUMP 16
#    endif

#    ifndef MADV_DODUMP
#        define MADV_DODUMP 17
#    endif
#endif

#if defined(_linux_)
#include <contrib/libs/numa/numaif.h>
#endif

#include <pthread.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <new>
#include <atomic>
#include <library/cpp/malloc/api/malloc.h>
#include <library/cpp/malloc/calloc/alloc_header.h>
#include <library/cpp/balloc_market/lib/alloc_stats.h>
#include <library/cpp/balloc_market/setup/alloc.h>

#include "balloc_defs.h"

#ifndef NDEBUG
#define DBG_FILL_MEMORY
#endif

#if defined(Y_COVER_PTR)
#define DBG_FILL_MEMORY
#endif

#if (defined(_i386_) || defined(_x86_64_)) && defined(_linux_)
#define HAVE_VDSO_GETCPU 1

#include <contrib/libs/linuxvdso/interface.h>
#endif

namespace NMarket::NBalloc {
#if HAVE_VDSO_GETCPU
    // glibc does not provide a wrapper around getcpu, we'll have to load it manually
    static int (*getcpu)(unsigned* cpu, unsigned* node, void* unused) = nullptr;
#endif

    static Y_FORCE_INLINE void* Advance(void* block, size_t size) {
        return (void*)((char*)block + size);
    }

    static constexpr size_t SINGLE_ALLOC = (SYSTEM_PAGE_SIZE / 2);
    static constexpr size_t ORDERS = 1024;
    static constexpr size_t DUMP_STAT = 0;
    static constexpr size_t PREPARE_AREA_MULTIPLIER = 128;
    static constexpr size_t MAX_PREPARE_ALLOCATION_MULTIPLIER = 4;

    static void* (*LibcMalloc)(size_t) = nullptr;
    static void (*LibcFree)(void*) = nullptr;

#define RDTSC(eax, edx) __asm__ __volatile__("rdtsc" \
                                             : "=a"(eax), "=d"(edx));
#define CPUID(func, eax, ebx, ecx, edx) __asm__ __volatile__("cpuid"                                      \
                                                             : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) \
                                                             : "a"(func));

    static int GetNumaNode() {
#if HAVE_VDSO_GETCPU
        if (Y_LIKELY(getcpu)) {
            unsigned node = 0;
            if (getcpu(nullptr, &node, nullptr)) {
                return 0;
            }
            return node;
        }
#endif
#if defined(_i386_) or defined(_x86_64_)
        int a = 0, b = 0, c = 0, d = 0;
        CPUID(0x1, a, b, c, d);
        int acpiID = (b >> 24);
        int numCPU = (b >> 16) & 255;
        if (numCPU == 0)
            return 0;
        int ret = acpiID / numCPU;
        return ret;
#else
        return 0;
#endif
    }


    static pthread_key_t key;
    static std::atomic<ui8> globalInitLock;
    static std::atomic<ui16> globalGCOrderCounter;


    class TLFAllocFreeList {
        struct TNode {
            TNode* Next;
        };

        // List of items available for allocations
        std::atomic<TNode*> Head;

        // List of locked items, to prevent ABA-problem they are locked because
        // one or more threads are allocating.
        std::atomic<TNode*> Pending;

        // This is the counter for moves from Pending to Head, to prevent ABA
        std::atomic<ui32> PendingToHeadCounter;

        // Number of allocations in progress
        std::atomic<ui32> AllocCount;

        // This is the list of items doomed for destroying, but locked
        // because one or more threads are allocating
        std::atomic<TNode*> Destroyed;

        static Y_FORCE_INLINE
        void Enqueue(std::atomic<TNode*>& list, TNode* n) {
            TNode* currHead = list.load(std::memory_order_relaxed);
            for (;;) {
                n->Next = currHead;
                if (list.compare_exchange_weak(currHead, n,
                        std::memory_order_release, std::memory_order_relaxed))
                    break;
            }
        }

        Y_FORCE_INLINE void* Dequeue() {
            TNode* currHead = Head.load(std::memory_order_acquire);
            while (currHead) {
                TNode* keepNext = currHead->Next;
                if (Head.compare_exchange_weak(currHead, keepNext,
                        std::memory_order_relaxed, std::memory_order_acquire)) {
                    // assert(keepNext == currHead->Next);
                    return currHead;
                }
            }
            return nullptr;
        }

        void EnqueueMany(TNode* fl) {
            if (!fl)
                return;
            TNode* flTail = fl;
            while (flTail->Next)
                flTail = flTail->Next;
            TNode* currHead = Head.load(std::memory_order_relaxed);
            for (;;) {
                flTail->Next = currHead;
                if (Head.compare_exchange_weak(currHead, fl,
                         std::memory_order_release, std::memory_order_relaxed))
                    break;
            }
        }

    public:
        Y_FORCE_INLINE void Free(void* ptr) {
            // If an application thread got the ptr via relaxed load
            // then it is required to add load fence here thus to be sure
            // that AllocCount loaded after the ptr loaded.
            // This is noop on intel (and compatible) platforms.
            std::atomic_thread_fence(std::memory_order_acquire);
            if (AllocCount.load(std::memory_order_acquire))
                // there are other threads currently allocating
                Enqueue(Pending, (TNode*)ptr);
            else
                // no allocations in progress
                Enqueue(Head, (TNode*)ptr);
        }

        Y_FORCE_INLINE void Destroy(void* ptr, size_t length) {
            // If an application thread got the ptr via relaxed load
            // then it is required to add load fence here thus to be sure
            // that AllocCount loaded after the ptr loaded.
            // This is noop on intel (and compatible) platforms.
            std::atomic_thread_fence(std::memory_order_acquire);
            if (AllocCount.load(std::memory_order_acquire)) {
                // there are other threads currently allocating
                Enqueue(Destroyed, (TNode*)ptr);
            } else {
                if (munmap(ptr, length) == -1) {
                    NMalloc::AbortFromCorruptedAllocator();
                }
            }
            CleanUpDestroyed(length);
        }

        Y_FORCE_INLINE void CleanUpDestroyed(size_t length) {
            ui32 keepCounter =
                PendingToHeadCounter.load(std::memory_order_acquire);
            TNode* fl = Destroyed.load(std::memory_order_acquire);
            if (fl == nullptr) {
                return;
            }
            for (;;) { // fake loop
                if (AllocCount.fetch_add(1, std::memory_order_seq_cst))
                    break;
                if (Y_UNLIKELY(keepCounter != PendingToHeadCounter.load(
                        std::memory_order_acquire)))
                    break;
                if (!Destroyed.compare_exchange_weak(fl, nullptr,
                         std::memory_order_release, std::memory_order_relaxed))
                    break;
                PendingToHeadCounter.store(keepCounter + 1,
                    std::memory_order_release);
                AllocCount.fetch_sub(1, std::memory_order_seq_cst);
                while (fl) {
                    TNode* next = fl->Next;
                    if (munmap(fl, length) == -1) {
                        NMalloc::AbortFromCorruptedAllocator();
                    }
                    fl = next;
                }
                return;
            }
            AllocCount.fetch_sub(1, std::memory_order_seq_cst);
        }

        Y_FORCE_INLINE void* TryToMovePendingToHead() {
            ui32 keepCounter =
                PendingToHeadCounter.load(std::memory_order_acquire);
            TNode* fl = Pending.load(std::memory_order_acquire);
            if (AllocCount.fetch_add(1, std::memory_order_seq_cst))
                return nullptr;
            // No other allocations in progress.
            if (!fl)
                return nullptr;
            if (Y_UNLIKELY(keepCounter != PendingToHeadCounter.load(
                    std::memory_order_acquire)))
                return nullptr;
            // If (keepCounter == PendingToHeadCounter) then Pending was not
            // freed by other threads. Hence Pending is not used in any
            // concurrent Alloc() ATM and can be safely moved to Head
            if (!Pending.compare_exchange_weak(fl, nullptr,
                     std::memory_order_release, std::memory_order_relaxed))
                return nullptr;
            PendingToHeadCounter.store(
                keepCounter + 1, std::memory_order_release);
            AllocCount.fetch_sub(1, std::memory_order_seq_cst);
            // pick first element from Pending and return it
            void* res = fl;
            fl = fl->Next;
            // Move all other elements from Pending to Head
            EnqueueMany(fl);
            return res;
        }

        Y_FORCE_INLINE void* Alloc() {
            // TryToMovePendingToHead() increments AllocCount
            void* res = TryToMovePendingToHead();
            if (res)
                return res;
            res = Dequeue();
            AllocCount.fetch_sub(1, std::memory_order_seq_cst);
            return res;
        }
    };

    TLFAllocFreeList nodes[2][ORDERS];
    std::atomic<ui64> sizesGC[2][16];
    std::atomic<ui64> sizeOS;
    std::atomic<ui64> totalOS;

    struct TBlockHeader {
        size_t Size;
        std::atomic<int> RefCount;
        unsigned short AllCount;
        unsigned short NumaNode;

        Y_FORCE_INLINE void MarkRefCountForSharedCache() {
            int expected = 0;
            bool cmpx = RefCount.compare_exchange_strong(
                expected, -1, std::memory_order_relaxed);
            if (Y_UNLIKELY(!cmpx))
                NMalloc::AbortFromCorruptedAllocator();
        }

        Y_FORCE_INLINE void UnmarkRefCountForSharedCache() {
            int expected = -1;
            bool cmpx = RefCount.compare_exchange_strong(
                expected, 0, std::memory_order_relaxed);
            if (Y_UNLIKELY(!cmpx))
                NMalloc::AbortFromCorruptedAllocator();
        }
    };

    struct TCacheNode {
        TCacheNode* Next;
    };

    static bool PushPage(void* page, size_t order) {
        if (order >= ORDERS)
            return false;
        int node = ((TBlockHeader*)page)->NumaNode;
        sizesGC[node][order % 16].fetch_add(order, std::memory_order_relaxed);
        TBlockHeader* blockHeader = (TBlockHeader*)page;
        blockHeader->MarkRefCountForSharedCache();
        nodes[node][order].Free(page);
        return true;
    }

    static void* PopPage(size_t order, ui16 numa) {
        if (order >= ORDERS)
            return nullptr;
        void* alloc = nodes[numa][order].Alloc();
        if (!alloc)
            return nullptr;
        sizesGC[numa][order % 16].fetch_sub(order, std::memory_order_relaxed);
        ((TBlockHeader*)alloc)->UnmarkRefCountForSharedCache();
        return alloc;
    }

#if DUMP_STAT
    static unsigned long long TickCounter() {
        int lo = 0, hi = 0;
        RDTSC(lo, hi);
        return (((unsigned long long)hi) << 32) + (unsigned long long)lo;
    }

    struct TTimeHold {
        unsigned long long Start;
        unsigned long long Finish;
        const char* Name;
        TTimeHold(const char* name)
            : Start(TickCounter())
            , Name(name)
        {
        }
        ~TTimeHold() {
            Finish = TickCounter();
            double diff = Finish > Start ? (Finish - Start) / 1000000.0 : 0.0;
            if (diff > 20.0) {
                fprintf(stderr, "%s %f mticks\n", diff, Name);
            }
        }
    };
#endif


#if DUMP_STAT
    std::atomic<i64> allocs[ORDERS];
#endif


    static void* Map(size_t size, bool allowPopulate = true) {
#if DUMP_STAT
        TTimeHold hold("mmap");
        size_t order = size / SYSTEM_PAGE_SIZE;
        if (order < ORDERS)
            allocs[order].fetch_add(1, std::memory_order_relaxed);
#endif
        if (!NAllocSetup::CanAlloc(
                 sizeOS.fetch_add(size, std::memory_order_relaxed) + size,
                 totalOS.load(std::memory_order_relaxed)))
        {
            NMalloc::AbortFromCorruptedAllocator();
        }

        int mmapProtFlags = PROT_READ | PROT_WRITE;
        int mmapFlags = MAP_PRIVATE | MAP_ANON;

#if defined(_linux_)
        if (allowPopulate && NAllocSetup::GetMapPopulateFlag()) {
            mmapFlags |= MAP_POPULATE;
        }
#else
        Y_UNUSED(allowPopulate);
#endif
        void* map = mmap(nullptr, size, mmapProtFlags, mmapFlags, -1, 0);
        if (map == MAP_FAILED) {
            NMalloc::AbortFromCorruptedAllocator();
        }
        NAllocStats::IncMmapCounter(size / SYSTEM_PAGE_SIZE);
        return map;
    }

    static void* AllocateFromPreparedArea(size_t size, ui16 numa);

    static void* SysAlloc(size_t& size) {
        size = AlignUp(size, SYSTEM_PAGE_SIZE);
        size_t order = size / SYSTEM_PAGE_SIZE;
        ui16 numa = GetNumaNode() & 1;
        void* result = PopPage(order, numa);
        if (result) {
            return result;
        }
        result = AllocateFromPreparedArea(size, numa);
        if (result) {
            return result;
        }
        result = PopPage(order, 1 - numa);
        if (result) {
            return result;
        }
        result = Map(size);
        ((TBlockHeader*)result)->NumaNode = numa;
        return result;
    }


    static void UnMap(void* block, size_t order) {
#if DUMP_STAT
        TTimeHold hold("munmap");
        if (order < ORDERS)
            allocs[order].fetch_sub(1, std::memory_order_relaxed);
#endif
        size_t size = order * SYSTEM_PAGE_SIZE;
        sizeOS.fetch_sub(size, std::memory_order_relaxed);
        TBlockHeader* blockHeader = (TBlockHeader*)block;
        blockHeader->MarkRefCountForSharedCache();
        if (order < ORDERS) {
            int node = blockHeader->NumaNode;
            nodes[node][order].Destroy(block, size);
        } else {
            if (munmap(block, size) == -1) {
                NMalloc::AbortFromCorruptedAllocator();
            }
        }
    }


    static void GCUnMap(void* block, size_t order);
    static void PrepareMemory(size_t);
    static void Destructor(void* data);


    static Y_COLD void GlobalInit() {
        // balloc has to be initialized, let's acquire globalInitLock
        ui8 expected = 0;
        if (!globalInitLock.compare_exchange_strong(expected, 1,
                 std::memory_order_relaxed))
        {
            // another thread is currently initializing balloc, let's wait
            while (globalInitLock.load(std::memory_order_acquire) < 2) {
                // empty loop
            }
            return;
        }

        // globalInitLock has been acquired, let's initialize balloc
#if HAVE_VDSO_GETCPU
        getcpu = (int (*)(unsigned*, unsigned*, void*))
            NVdso::Function("__vdso_getcpu", "LINUX_2.6");
#endif
        LibcMalloc = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
        LibcFree = (void (*)(void*))dlsym(RTLD_NEXT, "free");
        NAllocSetup::SetPrepareMemoryFunc(&PrepareMemory);
        pthread_key_create(&key, Destructor);

        // initialization complete, let's mark balloc is ready
        globalInitLock.store(2, std::memory_order_release);
    }


    enum EMode {
        Empty = 0,
        Born,
        Alive,
        Disabled,
        Dead,
        ToBeEnabled
    };

    struct TLS {
        TCacheNode* PageCacheList;
        size_t PageCacheCounter;
        void* Chunk;
        size_t Ptr;
        void* Block;
        int Counter;
        EMode Mode;
        unsigned char Count0;
        unsigned long Count1;
        ui16 numaCounterForSysClear;
        bool NeedGC() {
            if ((Count0 += 2) != 0)
                return false;
            totalOS.fetch_add(1, std::memory_order_relaxed);
            unsigned long long count = 0;
            for (size_t i = 0; i < 16; ++i) {
                count += sizesGC[0][i].load(std::memory_order_relaxed);
                count += sizesGC[1][i].load(std::memory_order_relaxed);
            }
            return NAllocSetup::NeedReclaim(count * SYSTEM_PAGE_SIZE, ++Count1);
        }
        void ClearCount() {
            Count1 = 0;
        }
    };

#if defined(_darwin_)

    static Y_FORCE_INLINE TLS* PthreadTls() {
        GlobalInit();
        TLS* ptls = (TLS*)pthread_getspecific(key);
        if (!ptls) {
            ptls = (TLS*)LibcMalloc(sizeof(TLS));
            if (!ptls) {
                NMalloc::AbortFromCorruptedAllocator(); // what do we do here?
            }
            memset(ptls, 0, sizeof(TLS));
            pthread_setspecific(key, ptls);
        }
        return ptls;
    }

#define tls (*PthreadTls())

#else

    __thread TLS tls;

#endif

    static void SysClear(size_t order, TLS& ltls) {
        void* page = PopPage(order, ltls.numaCounterForSysClear & 1);
        ++ltls.numaCounterForSysClear;
        if (page) {
            GCUnMap(page, order);
        } else {
            nodes[0][order].CleanUpDestroyed(order * SYSTEM_PAGE_SIZE);
            nodes[1][order].CleanUpDestroyed(order * SYSTEM_PAGE_SIZE);
        }
    }

    static constexpr size_t NUMBER_OF_SLOTS = 8;
    static constexpr size_t NUMBER_OF_NUMA_NODES = 2;
    struct TPreparedArea {
        /* The first bit (least significant) is a maintenance lock,
         * allocating thread should back off from the slot
         * if maintenance lock was set.
         * All other bits is a counter of allocating threads in the slot. */
        std::atomic<size_t> Lock;

        void* Area;
        size_t Size;
        unsigned short NumaNode;

        /* An allocating thread atomically increments the pivot
           thus allocating some memory pages from the slot */
        std::atomic<size_t> AcquiredPivot;

        /* The pivot mark prepared memory in the slot */
        size_t PreparedPivot;

        /* Garbage tail is set once by an allocating thread in case of
           the thread allocated last pages of memory at the tail of the area
           but the allocated block was too small */
        size_t GarbageTail;
    };

    std::atomic<bool> preparedMemoryReady;
    std::atomic<bool> preparedMemoryShutdownComplete = {true};
    std::atomic<bool> prepareLock;
    TPreparedArea prepareSlots[NUMBER_OF_SLOTS][NUMBER_OF_NUMA_NODES];
    std::atomic<size_t> activeSlot;

    static void* AllocateFromPreparedArea(size_t size, ui16 numa) {
        // let's check if prepared memory was ready for operation
        if (!preparedMemoryReady.load(std::memory_order_acquire))
            return nullptr;

        size_t currentActiveSlot =
            activeSlot.load(std::memory_order_acquire);

        for (size_t tryCount = 0; tryCount < NUMBER_OF_SLOTS; ++tryCount) {
            TPreparedArea& current = prepareSlots[currentActiveSlot][numa];
            // let's check if the slots is ready for operation
            if (current.Lock.load(std::memory_order_acquire) & 1) {
                // current slot is locked for maintenance
                // let's try the next one
                continue;
            }
            // let's lock current slot for memory allocation
            size_t lockState =
                current.Lock.fetch_add(2, std::memory_order_seq_cst);
            if (Y_UNLIKELY(lockState & 1)) {
                // current slot is locked for maintenance
                // let's try the next one
                current.Lock.fetch_sub(2, std::memory_order_seq_cst);
                continue;
            }

            if (Y_UNLIKELY(current.Area == nullptr)) {
                // current slot is not ready for some reason
                current.Lock.fetch_sub(2, std::memory_order_seq_cst);
                continue;
            }

            if (Y_UNLIKELY(current.Size < size * MAX_PREPARE_ALLOCATION_MULTIPLIER)) {
                /* allocation is too big */
                current.Lock.fetch_sub(2, std::memory_order_seq_cst);
                return nullptr;
            }

            size_t acquiredPivot = current.AcquiredPivot.fetch_add(size);
            if (Y_LIKELY(acquiredPivot + size <= current.Size)) {
                // a usable block has been allocated
                void* resultBlock = Advance(current.Area, acquiredPivot);
                current.Lock.fetch_sub(2, std::memory_order_seq_cst);
                ((TBlockHeader*)resultBlock)->NumaNode = numa;
                if (!NAllocSetup::CanAlloc(
                       sizeOS.fetch_add(size, std::memory_order_relaxed) + size,
                       totalOS.load(std::memory_order_relaxed)))
                {
                    NMalloc::AbortFromCorruptedAllocator();
                }
                return resultBlock;
            }

            // current slot is exhausted, let's try another one
            if (Y_UNLIKELY(acquiredPivot < current.Size)) {
                // unusable tail block has been acquired
                // let's mark it as a garbage
                // BTW: atomic is not neccessary here, because
                // synchronization is performed via Lock
                current.GarbageTail = acquiredPivot;
            }

            // let's try to close the slot for maintenance
            // allocating lock is still hold so
            // noway the slot has been reset from another thread
            current.Lock.fetch_or(1, std::memory_order_seq_cst);

            current.Lock.fetch_sub(2, std::memory_order_seq_cst);

            currentActiveSlot = (currentActiveSlot + 1) % NUMBER_OF_SLOTS;
        }
        return nullptr;
    }


    static void CleanUpSlot(TPreparedArea& current) {
        if (current.GarbageTail != current.Size) {
            // The slot was exhausted but
            // we have got some marked garbage
            void* blockToFree =
                Advance(current.Area, current.GarbageTail);
            size_t sizeToFree = current.Size - current.GarbageTail;
            int success = munmap(blockToFree, sizeToFree);
            if (success == -1)
                NMalloc::AbortFromCorruptedAllocator();
        } else {
            // We have already synchronized memory by setting slot
            // maintenance lock
            size_t acquiredPivot =
                current.AcquiredPivot.load(std::memory_order_relaxed);
            if (acquiredPivot < current.Size) {
                // we have got some unallocated memory
                void* blockToFree =
                    Advance(current.Area, acquiredPivot);
                size_t sizeToFree = current.Size - acquiredPivot;
                int success = munmap(blockToFree, sizeToFree);
                if (success == -1)
                    NMalloc::AbortFromCorruptedAllocator();
            }
        }
    }


    static void
    ResetSlot(size_t prepareSize, ui16 numa, TPreparedArea& current) {
        if (current.Area != nullptr)
            CleanUpSlot(current);
        current.Size = prepareSize * PREPARE_AREA_MULTIPLIER;

        int mmapProtFlags = PROT_READ | PROT_WRITE;
        int mmapFlags = MAP_PRIVATE | MAP_ANON | MAP_NORESERVE;

        current.Area =
            mmap(nullptr, current.Size, mmapProtFlags, mmapFlags, -1, 0);
        if (current.Area == MAP_FAILED) {
            NMalloc::AbortFromCorruptedAllocator();
        }

#if defined(_linux_)
        ui64 nodeMask = 1UL << numa;
        mbind(current.Area, current.Size, MPOL_PREFERRED,
            &nodeMask, sizeof(nodeMask), 0);
        int success = madvise(current.Area, current.Size, MADV_DONTDUMP);
        if (success == -1) {
            NMalloc::AbortFromCorruptedAllocator();
        }
        if (NAllocSetup::GetHugePagesInPreparedMemory())
            // don't really care about success
            madvise(current.Area, current.Size, MADV_HUGEPAGE);
#endif

        current.AcquiredPivot.store(0, std::memory_order_relaxed);
        current.PreparedPivot = 0;
        current.GarbageTail = current.Size;
        current.NumaNode = numa;
    }


    static bool InitPreparedAreas(size_t prepareSize) {
        // let's lock all slots for maintenance
        // and find a slot without other operating threads
        size_t choosedArea = NUMBER_OF_SLOTS;
        for (ui16 numa = 0; numa < NUMBER_OF_NUMA_NODES; ++numa) {
            for (size_t i = 0; i < NUMBER_OF_SLOTS; ++i) {
                size_t lockState = prepareSlots[i][numa].Lock.fetch_or(
                    1, std::memory_order_seq_cst);
                if (lockState == 0 || lockState == 1) {
                    if (choosedArea == NUMBER_OF_SLOTS)
                        choosedArea = i;
                }
            }

            if (choosedArea == NUMBER_OF_SLOTS) {
                // we could not find an area without active threads, let's give up
                return false;
            }

            activeSlot.store(choosedArea, std::memory_order_release);
            TPreparedArea& current = prepareSlots[choosedArea][numa];
            ResetSlot(prepareSize, numa, current);
            current.Lock.fetch_sub(1, std::memory_order_seq_cst);

            preparedMemoryShutdownComplete.store(
                false, std::memory_order_relaxed);
        }
        preparedMemoryReady.store(true, std::memory_order_release);
        return true;
    }


    static size_t
    PrepareMemoryInSlot(size_t prepareSize, size_t slot, size_t numa) {
        TPreparedArea& current = prepareSlots[slot][numa];
        size_t acquiredPivot =
            current.AcquiredPivot.load(std::memory_order_acquire);

        if (current.Area == nullptr || acquiredPivot >= current.Size) {
            // the slot is already exhausted
            // let's do some maintenance
            size_t lockState = 0;
            bool locked = current.Lock.compare_exchange_strong(
                lockState, 1, std::memory_order_seq_cst);
            if (!locked && lockState != 1) {
                // there is some allocating threads in the slot
                // can do nothing here but mark the slot for maintenance
                if ((lockState & 1) == 0)
                    current.Lock.fetch_or(1, std::memory_order_seq_cst);
                return 0;
            }
            // the slot is under maintenance lock and there is not acquiring
            // threads in the slot
            ResetSlot(prepareSize, numa, current);
            acquiredPivot = 0;
        }

        size_t targetPreparePivot = acquiredPivot + prepareSize;
        if (targetPreparePivot > current.Size) {
            prepareSize -= targetPreparePivot - current.Size;
            targetPreparePivot = current.Size;
        }

        if (current.PreparedPivot >= targetPreparePivot)
            return prepareSize;

        int success;
        int advice = MADV_WILLNEED;
        if (current.PreparedPivot >= acquiredPivot) {
            success = madvise(
                Advance(current.Area, current.PreparedPivot),
                targetPreparePivot - current.PreparedPivot,
                advice);
        } else {
            success = madvise(
                Advance(current.Area, acquiredPivot),
                targetPreparePivot - acquiredPivot,
                advice);
        }
        if (success == -1)
            NMalloc::AbortFromCorruptedAllocator();

#if defined(_linux_)
        if (current.PreparedPivot >= acquiredPivot) {
            success = madvise(
                Advance(current.Area, current.PreparedPivot),
                targetPreparePivot - current.PreparedPivot,
                MADV_DODUMP);
        } else {
            success = madvise(
                Advance(current.Area, acquiredPivot),
                targetPreparePivot - acquiredPivot,
                MADV_DODUMP);
        }
        if (success == -1)
            NMalloc::AbortFromCorruptedAllocator();
#endif

        current.PreparedPivot = targetPreparePivot;

        // let's clear maintenance lock
        if (current.Lock.load(std::memory_order_acquire) & 1)
            current.Lock.fetch_sub(1, std::memory_order_seq_cst);

        return prepareSize;
    }


    static void CleanUpGarbageBlocks();


    static void PrepareMmapedArea(size_t prepareSize) {
        if (prepareLock.exchange(true, std::memory_order_seq_cst))
            // another thread is preparing memory, let's back off
            return;
        if (!preparedMemoryReady.load(std::memory_order_acquire)) {
            // prepared areas are not ready yet
            if (!InitPreparedAreas(prepareSize)) {
                prepareLock.store(false, std::memory_order_release);
                return;
            }
        }

        CleanUpGarbageBlocks();

        prepareSize = AlignUp(prepareSize, SYSTEM_PAGE_SIZE);

        size_t slot = activeSlot.load(std::memory_order_acquire);
        bool firstFound = false;
        // let's find some slots to prepare memory
        for (ui16 numa = 0; numa < NUMBER_OF_NUMA_NODES; ++numa) {
            size_t numaPrepareSize = prepareSize;
            for (size_t tryCount = 0;
                 tryCount < NUMBER_OF_SLOTS * 2;
                 ++tryCount)
            {
                size_t trySlot = (slot + tryCount) % NUMBER_OF_SLOTS;
                size_t prepared =
                    PrepareMemoryInSlot(numaPrepareSize, trySlot, numa);
                numaPrepareSize -= prepared;
                if (!firstFound && prepared != 0) {
                    firstFound = true;
                    if (tryCount != 0)
                        activeSlot.store(trySlot, std::memory_order_release);
                }
                if (numaPrepareSize == 0)
                    break;
            }

            // let's lock some slots for maintenance
            for (size_t tryCount = 0; tryCount < NUMBER_OF_SLOTS; ++tryCount) {
                size_t trySlot = (slot + tryCount) % NUMBER_OF_SLOTS;
                TPreparedArea& current = prepareSlots[trySlot][numa];
                if (current.Lock.load(std::memory_order_acquire) & 1)
                    continue; // the slot is already under maintenance lock
                if (current.Area != nullptr) {
                    size_t acquiredPivot =
                        current.AcquiredPivot.load(std::memory_order_acquire);
                    if (acquiredPivot < current.Size)
                        continue; // the slot has some free memory
                }
                current.Lock.fetch_or(1, std::memory_order_seq_cst);
            }
        }

        prepareLock.store(false, std::memory_order_release);
    }


    static void ShutdownPreparedMemory() {
        if (preparedMemoryShutdownComplete.load(std::memory_order_acquire))
            return;
        if (prepareLock.exchange(true, std::memory_order_seq_cst))
            return;

        preparedMemoryReady.store(false, std::memory_order_release);

        bool complete = true;
        for (ui16 numa = 0; numa < NUMBER_OF_NUMA_NODES; ++numa) {
            for (size_t i = 0; i < NUMBER_OF_SLOTS; ++i) {
                TPreparedArea& current = prepareSlots[i][numa];
                size_t lockState =
                    current.Lock.fetch_or(1, std::memory_order_seq_cst);
                if (current.Area == nullptr)
                    continue;
                if (lockState == 0 || lockState == 1) {
                    CleanUpSlot(current);
                    current.Area = nullptr;
                    current.GarbageTail = 0;
                    current.PreparedPivot = 0;
                    current.Size = 0;
                    current.NumaNode = 0;
                    current.AcquiredPivot.store(0, std::memory_order_relaxed);
                } else {
                    complete = false;
                }
            }
        }
        if (complete)
            preparedMemoryShutdownComplete.store(
                true, std::memory_order_release);

        // should be after setting preparedMemoryShutdownComplete
        CleanUpGarbageBlocks();

        prepareLock.store(false, std::memory_order_release);
    }


    static void PrepareMemory(size_t) {
        size_t prepareSize = NAllocSetup::GetPreparedMemorySizeInBytes();
        if (prepareSize) {
            PrepareMmapedArea(prepareSize);
        } else {
            ShutdownPreparedMemory();
        }
    }


    struct TGarbageBlockNode {
        TBlockHeader Protect;
        std::atomic<TGarbageBlockNode*> Next;
    };

    class TGarbageQueue {
    public:
        TGarbageQueue() {
            FakeNode.Next.store(nullptr, std::memory_order_relaxed);
            Head = &FakeNode;
            Tail.store(&FakeNode, std::memory_order_relaxed);
        }

        void enqueue(void* block) {
            TGarbageBlockNode* next = (TGarbageBlockNode*)block;
            next->Next.store(nullptr, std::memory_order_relaxed);
            TGarbageBlockNode* prev =
                Tail.exchange(next, std::memory_order_seq_cst);
            prev->Next.store(next, std::memory_order_release);
        }

        void* dequeue() {
            for (;;) {
                TGarbageBlockNode* nextHead =
                    Head->Next.load(std::memory_order_acquire);
                if (nextHead == nullptr)
                    return nullptr;
                TGarbageBlockNode* result = Head;
                Head = nextHead;
                if (result != &FakeNode)
                    return result;
                enqueue(result);
            }
        }

    private:
        TGarbageBlockNode FakeNode;
        TGarbageBlockNode* Head;
        std::atomic<TGarbageBlockNode*> Tail;
    };

    TGarbageQueue garbageQueue;


    static void GCUnMap(void* block, size_t order) {
        // Here we must be sure that there is no way madvise
        // and munmap are called on the same address space at the same time.
        // So we must be sure that AcquiredPivot (of the slot
        // from which the block was allocated) is read by preparing thread
        // after the block was allocated
        if (preparedMemoryReady.load(std::memory_order_acquire)) {
            // if prepared memory is currently operating
            // then we better put the block to the GC queue
            // thus we are not going to call munmap syscall
            ((TBlockHeader*)block)->Size = order * SYSTEM_PAGE_SIZE;
            garbageQueue.enqueue(block);

            if (!preparedMemoryReady.load(std::memory_order_acquire)) {
                // we are shutting down prepared memory logic
                // let's clean it up
                if (!prepareLock.exchange(true, std::memory_order_seq_cst)) {
                    CleanUpGarbageBlocks();
                    prepareLock.store(false, std::memory_order_release);
                } else {
                    // can't clean up garbage blocks, let's clear
                    // preparedMemoryShutdownComplete flag
                    // so the next ShutdownPreparedMemory will clean it up
                    preparedMemoryShutdownComplete.store(
                        false, std::memory_order_release);
                }
            }
        } else {
            // if preparedMemoryReady was false then all subsequent madvise
            // calls are safe
            UnMap(block, order);
        }
    }


    static void CleanUpGarbageBlocks() {
        while (void* block = garbageQueue.dequeue()) {
            UnMap(block, ((TBlockHeader*)block)->Size / SYSTEM_PAGE_SIZE);
        }
    }


    static void UnRefHard(void* block, int add, TLS& ltls) {
        TBlockHeader* blockHeader = (TBlockHeader*)block;

        auto& refCount = blockHeader->RefCount;
        if (refCount.load(std::memory_order_acquire) != add) {
            if (refCount.fetch_sub(add, std::memory_order_acquire) != add)
                return;
        } else {
            refCount.store(0, std::memory_order_relaxed);
        }

        size_t order = blockHeader->Size / SYSTEM_PAGE_SIZE;

        if (blockHeader->Size % SYSTEM_PAGE_SIZE) {
            NMalloc::AbortFromCorruptedAllocator();
        }

        if (ltls.Mode != Alive) {
            if (Y_UNLIKELY(ltls.Mode == Disabled)
                    && NAllocSetup::IsBallocForeverAndOnly())
            {
                NMalloc::AbortFromCorruptedAllocator();
            }
            if (Y_UNLIKELY(ltls.Mode == Disabled) || !PushPage(block, order)) {
                GCUnMap(block, order);
            }
            return;
        }

        if (ltls.NeedGC()) {
            ltls.ClearCount();
            size_t index =
                globalGCOrderCounter.fetch_add(1, std::memory_order_relaxed);
            size_t orderForGC = index % (ORDERS - 16);
            if (orderForGC > 64) {
                orderForGC = AlignUp(orderForGC, 16);
            }
            if (!NAllocSetup::IsTooSmallToUnmap(orderForGC)) {
                SysClear(orderForGC, ltls);
            }
            if (!NAllocSetup::IsTooSmallToUnmap(order)) {
                GCUnMap(block, order);
                return;
            }
        }

        for (;;) { // fake for-loop
            if (order != 1)
                break;
            size_t cacheSizeLimit =
                NAllocSetup::GetThreadLocalCacheSizeInBytes();
            size_t expectedCacheSize =
                (ltls.PageCacheCounter + 1) * SYSTEM_PAGE_SIZE;
            if (expectedCacheSize > cacheSizeLimit)
                break;

            // the limit is ok, let's add the block to the cache
            TCacheNode* newNode = (TCacheNode*)block;
            newNode->Next = ltls.PageCacheList;
            ltls.PageCacheList = newNode;
            ++ltls.PageCacheCounter;
            return;
        }

        if (Y_UNLIKELY(ltls.Mode == Disabled) || !PushPage(block, order)) {
            GCUnMap(block, order);
        }
    }


    static Y_COLD void Init() {
        TLS& ltls = tls;
        bool ShouldEnable =
            (NAllocSetup::IsEnabledByDefault() || ltls.Mode == ToBeEnabled);
        ltls.Mode = Born;
        if (Y_UNLIKELY(globalInitLock.load(std::memory_order_acquire) != 2))
            GlobalInit();
        pthread_setspecific(key, (void*)&ltls);
        if (ShouldEnable) {
            ltls.Mode = Alive;
        } else {
            ltls.Mode = Disabled;
        }
    }


    static void Y_FORCE_INLINE UnRef(void* block, int counter, TLS& ltls) {
        if (Y_UNLIKELY(ltls.Mode != Alive)) {
            UnRefHard(block, counter, ltls);
            return;
        }
        if (ltls.Block == block) {
            ltls.Counter += counter;
        } else {
            if (ltls.Block) {
                UnRefHard(ltls.Block, ltls.Counter, ltls);
            }
            ltls.Block = block;
            ltls.Counter = counter;
        }
    }

    static void Destructor(void* data) {
        TLS& ltls = *(TLS*)data;
        ltls.Mode = Dead;
        if (ltls.Chunk) {
            TBlockHeader* blockHeader = (TBlockHeader*)ltls.Chunk;
            UnRef(ltls.Chunk, SYSTEM_PAGE_SIZE - blockHeader->AllCount, ltls);
        }
        if (ltls.Block) {
            UnRef(ltls.Block, ltls.Counter, ltls);
        }
        for (TCacheNode* cacheNode = ltls.PageCacheList; cacheNode != nullptr;) {
            TCacheNode* next = cacheNode->Next;
            ((TBlockHeader*)(void*)cacheNode)->RefCount.store(0,
                std::memory_order_relaxed);
            PushPage(cacheNode, 1);
            cacheNode = next;
        }
#if defined(_darwin_)
        LibcFree(data);
#endif
    }

    using TAllocHeader = NCalloc::TAllocHeader;


    static Y_FORCE_INLINE
    void ConditionalFillMemory(void* dst, char pattern, size_t size) {
#ifdef DBG_FILL_MEMORY
        memset(dst, pattern, size);
#else
        Y_UNUSED(dst);
        Y_UNUSED(pattern);
        Y_UNUSED(size);
#endif
    }


    static Y_FORCE_INLINE
    void* AllocateRawOnly(size_t size) {
        TLS& ltls = tls;
        size = AlignUp(size, NCalloc::ALLOC_HEADER_ALIGNMENT);
        size_t extsize = size + sizeof(TBlockHeader);

        size_t ptr = ltls.Ptr;
        void* chunk = ltls.Chunk;

        if (Y_LIKELY(ptr >= extsize)) {
            ptr = ptr - size;
            void* payload = (TAllocHeader*)Advance(chunk, ptr);
            TBlockHeader* blockHeader = (TBlockHeader*)chunk;
            ++blockHeader->AllCount;
            ltls.Ptr = ptr;
            if (NAllocStats::IsEnabled())
                NAllocStats::IncThreadAllocStats(size);
            ConditionalFillMemory(payload, 0xec, size);
            // see ABOUT STORE AND LOAD FENCES above
            std::atomic_thread_fence(std::memory_order_release);
            return payload;
        }

        if (extsize > SINGLE_ALLOC) {
            // The dlsym() function in GlobalInit() may call malloc() resulting in recursive call
            // of the NBalloc::Malloc(). We have to serve such allocation request via balloc even
            // when (IsEnabledByDefault() == false) because at this point we don't know where the
            // libc malloc is.
            if (extsize > 64 * SYSTEM_PAGE_SIZE) {
                extsize = AlignUp(extsize, 16 * SYSTEM_PAGE_SIZE);
            }
            NAllocSetup::ThrowOnError(extsize);
            // Sysalloc aligns extsize up to SYSTEM_PAGE_SIZE
            void* block = SysAlloc(extsize);
            TBlockHeader* blockHeader = (TBlockHeader*)block;

            blockHeader->RefCount.store(1, std::memory_order_relaxed);
            blockHeader->Size = extsize;
            blockHeader->AllCount = 0;
            void* payload = Advance(block, sizeof(TBlockHeader));

            if (NAllocStats::IsEnabled())
                NAllocStats::IncThreadAllocStats(size);

            ConditionalFillMemory(payload, 0xec, size);

            // see ABOUT STORE AND LOAD FENCES above
            std::atomic_thread_fence(std::memory_order_release);
            return payload;
        }

        if (ptr < extsize) {
            NAllocSetup::ThrowOnError(SYSTEM_PAGE_SIZE);
            if (chunk) {
                TBlockHeader* blockHeader = (TBlockHeader*)chunk;
                UnRef(chunk, SYSTEM_PAGE_SIZE - blockHeader->AllCount, ltls);
            }
            void* block = nullptr;
            ui16 numa = GetNumaNode() & 1;
            while (1) {
                if (ltls.PageCacheList != nullptr) {
                    block = ltls.PageCacheList;
                    ltls.PageCacheList = ltls.PageCacheList->Next;
                    --ltls.PageCacheCounter;
                    break;
                }
                block = PopPage(1, numa);
                if (block) {
                    break;
                }
                block = AllocateFromPreparedArea(SYSTEM_PAGE_SIZE, numa);
                if (block) {
                    break;
                }
                block = PopPage(1, 1 - numa);
                if (block) {
                    break;
                }

                size_t cacheSize = AlignUp(
                    NAllocSetup::GetThreadLocalCacheSizeInBytes(),
                    SYSTEM_PAGE_SIZE);
                void* mmapArea = Map(cacheSize);
                for (size_t i = 0; i < cacheSize / SYSTEM_PAGE_SIZE; ++i) {
                    ((TBlockHeader*)mmapArea)->NumaNode = numa;
                    ((TCacheNode*)mmapArea)->Next = ltls.PageCacheList;
                    ltls.PageCacheList = (TCacheNode*)mmapArea;
                    mmapArea = Advance(mmapArea, SYSTEM_PAGE_SIZE);
                }
                ltls.PageCacheCounter = cacheSize / SYSTEM_PAGE_SIZE;
            }
            TBlockHeader* blockHeader = (TBlockHeader*)block;
            blockHeader->RefCount.store(SYSTEM_PAGE_SIZE,
                std::memory_order_relaxed);
            blockHeader->Size = SYSTEM_PAGE_SIZE;
            blockHeader->AllCount = 0;
            ltls.Ptr = SYSTEM_PAGE_SIZE;
            ltls.Chunk = block;
            ptr = ltls.Ptr;
            chunk = ltls.Chunk;
        }

        ptr = ptr - size;
        void* payload = Advance(chunk, ptr);
        TBlockHeader* blockHeader = (TBlockHeader*)chunk;
        ++blockHeader->AllCount;
        ltls.Ptr = ptr;
        if (NAllocStats::IsEnabled()) {
            NAllocStats::IncThreadAllocStats(size);
        }

        ConditionalFillMemory(payload, 0xec, size);

        // see ABOUT STORE AND LOAD FENCES above
        std::atomic_thread_fence(std::memory_order_release);
        return payload;
    }


    static Y_FORCE_INLINE
    TAllocHeader* AllocateRaw(size_t size, size_t signature) {
        TLS& ltls = tls;
        const size_t allocHeaderSize = TAllocHeader::GetHeaderSize();
        size = AlignUp(size, NCalloc::ALLOC_HEADER_ALIGNMENT);
        size_t extsize = size + allocHeaderSize + sizeof(TBlockHeader);

        size_t ptr = ltls.Ptr;
        void* chunk = ltls.Chunk;

        if (Y_LIKELY(ptr >= extsize)) {
            ptr = ptr - size - allocHeaderSize;
            TAllocHeader* allocHeader = (TAllocHeader*)Advance(chunk, ptr);
            allocHeader->Encode(chunk, size, signature);
            TBlockHeader* blockHeader = (TBlockHeader*)chunk;
            ++blockHeader->AllCount;
            ltls.Ptr = ptr;
            if (NAllocStats::IsEnabled())
                NAllocStats::IncThreadAllocStats(size);
            ConditionalFillMemory(allocHeader->GetDataPtr(), 0xec, size);
            // see ABOUT STORE AND LOAD FENCES above
            std::atomic_thread_fence(std::memory_order_release);
            return allocHeader;
        }

        if (extsize > SINGLE_ALLOC) {
            // The dlsym() function in GlobalInit() may call malloc() resulting in recursive call
            // of the NBalloc::Malloc(). We have to serve such allocation request via balloc even
            // when (IsEnabledByDefault() == false) because at this point we don't know where the
            // libc malloc is.
            if (extsize > 64 * SYSTEM_PAGE_SIZE) {
                extsize = AlignUp(extsize, 16 * SYSTEM_PAGE_SIZE);
            }
            NAllocSetup::ThrowOnError(extsize);
            // Sysalloc aligns extsize up to SYSTEM_PAGE_SIZE
            void* block = SysAlloc(extsize);
            TBlockHeader* blockHeader = (TBlockHeader*)block;

            blockHeader->RefCount.store(1, std::memory_order_relaxed);
            blockHeader->Size = extsize;
            blockHeader->AllCount = 0;
            TAllocHeader* allocHeader = (TAllocHeader*)Advance(block, sizeof(TBlockHeader));
            allocHeader->Encode(blockHeader, size, signature);

            if (NAllocStats::IsEnabled())
                NAllocStats::IncThreadAllocStats(size);

            ConditionalFillMemory(allocHeader->GetDataPtr(), 0xec, size);

            // see ABOUT STORE AND LOAD FENCES above
            std::atomic_thread_fence(std::memory_order_release);
            return allocHeader;
        }

        if (ptr < extsize) {
            NAllocSetup::ThrowOnError(SYSTEM_PAGE_SIZE);
            if (chunk) {
                TBlockHeader* blockHeader = (TBlockHeader*)chunk;
                UnRef(chunk, SYSTEM_PAGE_SIZE - blockHeader->AllCount, ltls);
            }
            void* block = nullptr;
            ui16 numa = GetNumaNode() & 1;
            while (1) {
                if (ltls.PageCacheList != nullptr) {
                    block = ltls.PageCacheList;
                    ltls.PageCacheList = ltls.PageCacheList->Next;
                    --ltls.PageCacheCounter;
                    break;
                }
                block = PopPage(1, numa);
                if (block) {
                    break;
                }
                block = AllocateFromPreparedArea(SYSTEM_PAGE_SIZE, numa);
                if (block) {
                    break;
                }
                block = PopPage(1, 1 - numa);
                if (block) {
                    break;
                }

                size_t cacheSize = AlignUp(
                    NAllocSetup::GetThreadLocalCacheSizeInBytes(),
                    SYSTEM_PAGE_SIZE);
                void* mmapArea = Map(cacheSize);
                for (size_t i = 0; i < cacheSize / SYSTEM_PAGE_SIZE; ++i) {
                    ((TBlockHeader*)mmapArea)->NumaNode = numa;
                    ((TCacheNode*)mmapArea)->Next = ltls.PageCacheList;
                    ltls.PageCacheList = (TCacheNode*)mmapArea;
                    mmapArea = Advance(mmapArea, SYSTEM_PAGE_SIZE);
                }
                ltls.PageCacheCounter = cacheSize / SYSTEM_PAGE_SIZE;
            }
            TBlockHeader* blockHeader = (TBlockHeader*)block;
            blockHeader->RefCount.store(SYSTEM_PAGE_SIZE,
                std::memory_order_relaxed);
            blockHeader->Size = SYSTEM_PAGE_SIZE;
            blockHeader->AllCount = 0;
            ltls.Ptr = SYSTEM_PAGE_SIZE;
            ltls.Chunk = block;
            ptr = ltls.Ptr;
            chunk = ltls.Chunk;
        }

        ptr = ptr - size - allocHeaderSize;
        TAllocHeader* allocHeader = (TAllocHeader*)Advance(chunk, ptr);
        allocHeader->Encode(chunk, size, signature);
        TBlockHeader* blockHeader = (TBlockHeader*)chunk;
        ++blockHeader->AllCount;
        ltls.Ptr = ptr;
        if (NAllocStats::IsEnabled()) {
            NAllocStats::IncThreadAllocStats(size);
        }

        ConditionalFillMemory(allocHeader->GetDataPtr(), 0xec, size);

        // see ABOUT STORE AND LOAD FENCES above
        std::atomic_thread_fence(std::memory_order_release);
        return allocHeader;
    }

    static void Y_FORCE_INLINE FreeRaw(void* ptr) {
        UnRef(ptr, 1, tls);
    }
}  // namespace NMarket::NBalloc
