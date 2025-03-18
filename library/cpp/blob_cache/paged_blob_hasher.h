#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/algorithm.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/string/cast.h>
#include <util/memory/blob.h>

#include <array>

// Thread-safe blob cache with static memory allocation.
// For stream processing applications like basesearch.
//
// Keeps ring buffer of fixed size pages that are used to
// store both keys and values.
//
// Values are of type TBlob. TKey is a parameter, it is assumed
// that TKey has small static size and can be efficiently reallocated
// into given memory region.
//
// To access cache, a thread locks 1 or more pages by
// calling AllocPage. Thread has exclusive right to
// update its pages. All "store" operations from this
// thread save data in its current active page.
// "Load" operations may choose to refresh an older
// cache entry by reallocating it into thread's
// active page (that is typically closer to the buffer head).
//
// When ring buffer overlaps, older pages at its tail are
// flushed. All entries that still keep data there are
// invalidated (possibly together with some entries that
// were recently refreshed).
//
// Thread safety is ensured by per-entry locks.
// Thread may fail to acquire entry lock which is
// interpreted as cache miss. Global locks (page, hasher)
// are only used for rare update operations, like page
// allocation and flush.
//
// Page allocation may fail in case of cache overflow, i.e.
// if there is a still locked page near the tail of ring buffer.
// May also happen if a thread forgets to delete its page object.
//

// Interface
//

struct THasherStats {
    // Page events
    size_t NumMissLock = 0;
    size_t NumMissCold = 0;
    size_t NumMissHot = 0;
    size_t NumHit = 0;
    size_t NumFlushConflict = 0;
    size_t NumStore = 0;
    size_t NumStoreMiss = 0;
    size_t NumRefresh = 0;
    size_t NumSkipRefresh = 0;
    size_t NumOverflowFail = 0;
    // Hasher events
    size_t NumPageFail = 0;
    size_t NumPageFlush = 0;
    // Size stats
    size_t HasherSize = 0;
    float PairsSizeFrac = 0.0;
    float PagesSizeFrac = 0.0;
    float BytesSizeFrac = 0.0;
    float SlotsSizeFrac = 0.0;
    // Utilization stats
    float BytesUtilization = 0.0;
    float SlotsUtilization = 0.0;
    float BytesWaste = 0.0;
    float SlotsWaste = 0.0;
    // Misc stats
    size_t NumSkippedPages = 0;
};

template <typename TKey>
class IPagedBlobHasher {
public:
    class IPage {
    public:
        virtual ~IPage() = default;
        virtual bool GetBlobOrFail(const TKey& key, TBlob& value) = 0; // value is valid while IPage lives
        virtual bool StoreBlobOrFail(const TKey& key, const TBlob& value) = 0;
        virtual size_t Avail() = 0;
        virtual bool HasEnoughSpace(const TKey& key, const TBlob& value) = 0; // doesn't guarantee success
    };

    using TPageUniqPtr = TAutoPtr<IPage>;

    class IPagesHolder {
    public:
        virtual ~IPagesHolder() = default;
        virtual bool GetBlobOrFail(const TKey& key, TBlob& value) = 0;
        virtual bool StoreBlobOrFail(const TKey& key, const TBlob& value) = 0;
    };

    using TPagesHolderUniqPtr = TAutoPtr<IPagesHolder>;

public:
    virtual ~IPagedBlobHasher() = default;
    virtual TPageUniqPtr AllocPage(size_t num = 1) = 0;                // may return null
    virtual TPagesHolderUniqPtr AllocPagesHolder(size_t maxPages) = 0; // returns valid ptr
    virtual void CalcStats(THasherStats& stats) = 0;                   // skips events from active pages
    virtual size_t GetSize() = 0;
    virtual void Suspend() = 0; // stop page allocation
    virtual void Resume() = 0;  // resume page allocation
};

namespace NPagedBlobHasherDefaults {
    struct TBlobHasherTraits {
        enum {
            NumPairs = 1000 * 1000,
            NumPages = 1000,
            PageSizeInKb = 20,
            NumPageSlots = 400,
            RetryCount = 100
        };
    };

    template <typename TKey>
    struct TNoOpKeyCopier {
        Y_FORCE_INLINE size_t GetSize(const TKey& /*key*/) const {
            return 0;
        }

        Y_FORCE_INLINE TKey Realloc(const TKey& key, char* /*buf*/) const {
            return key;
        }
    };

    struct TBlobCopier {
        Y_FORCE_INLINE size_t GetSize(const TBlob& key) const {
            return key.Size();
        }

        Y_FORCE_INLINE TBlob Realloc(const TBlob& key, char* buf) const {
            memcpy(buf, key.AsCharPtr(), key.Size());
            return TBlob::NoCopy(buf, key.Size());
        }
    };

    struct TStringBufCopier {
        Y_FORCE_INLINE size_t GetSize(const TStringBuf& key) const {
            return key.size();
        }

        Y_FORCE_INLINE TStringBuf Realloc(const TStringBuf& key, char* buf) const {
            memcpy(buf, key.data(), key.size());
            return TStringBuf(buf, key.size());
        }
    };

}

template <typename TKey,
          typename TKeyCopier,
          typename TKeyEq,
          typename TKeyHash,
          typename TTraits>
class TPagedBlobHasher;

// Implementation
//

namespace NPagedBlobHasherPrivate {
    template <size_t RetryCount>
    Y_FORCE_INLINE bool GetLock(TAtomic& lock) {
        static_assert(RetryCount > 0, "retry count should be > 0");

        for (size_t i = 0; i < RetryCount; ++i) {
            if (AtomicCas(&lock, 1, 0)) {
                return true;
            }
        }
        return false;
    }

    Y_FORCE_INLINE bool IsLocked(TAtomic& lock) {
        return AtomicGet(lock) == 1;
    }

    Y_FORCE_INLINE void AcquireLock(TAtomic& lock) {
        while (!AtomicCas(&lock, 1, 0)) {
        }
    }

    Y_FORCE_INLINE void ReleaseLock(TAtomic& lock) {
        Y_ASSERT(lock == 1);
        AtomicSet(lock, 0);
    }

    using THashType = ui64;
    using TIndexType = ui32;

    static_assert(~(THashType)0 > 0, "hash type should be unsigned");
    static_assert(~(TIndexType)0 > 0, "index type should be unsigned");

    constexpr TIndexType MaxIndex = ~(TIndexType)0;

    template <typename TKey,
              size_t RetryCount>
    class THashPair {
    public:
        TKey Key;
        TBlob Value;
        bool WasValid = false;

    public:
        Y_FORCE_INLINE bool GetLock() {
            return NPagedBlobHasherPrivate::GetLock<RetryCount>(PairLock);
        }

        Y_FORCE_INLINE bool IsLocked() {
            return NPagedBlobHasherPrivate::IsLocked(PairLock);
        }

        Y_FORCE_INLINE void ReleaseLock() {
            NPagedBlobHasherPrivate::ReleaseLock(PairLock);
        }

        Y_FORCE_INLINE TIndexType GetPageIndex() {
            return AtomicGet(PageIndex);
        }

        Y_FORCE_INLINE bool HasPageIndex(TIndexType pageIndex) {
            Y_ASSERT(pageIndex != MaxIndex);
            return AtomicGet(PageIndex) == pageIndex;
        }

        Y_FORCE_INLINE bool IsValid() {
            return AtomicGet(PageIndex) != MaxIndex;
        }

        Y_FORCE_INLINE void SetPageIndex(TIndexType pageIndex) {
            Y_ASSERT(pageIndex != MaxIndex);
            AtomicSet(PageIndex, pageIndex);
        }

        Y_FORCE_INLINE void Invalidate() {
            AtomicSet(PageIndex, MaxIndex);
        }

    private:
        TAtomic PageIndex = MaxIndex; // for lock-free invalidate
        TAtomic PairLock = 0;
    };

    class TEventStats {
    public:
        enum EEvent {
            Hit,
            MissLock,
            MissCold,
            MissHot,
            FlushConflict,
            Store,
            StoreMiss,
            Refresh,
            SkipRefresh,
            OverflowFail,
            PageFail,
            PageFlush,
            NumEvents
        };

        TEventStats() {
            Fill(Counts.begin(), Counts.end(), 0);
        }

        void Inc(EEvent event) {
            Counts[event] += 1;
        }

        size_t Get(EEvent event) {
            return Counts[event];
        }

        TEventStats& operator+=(const TEventStats& oth) {
            for (size_t i = 0; i != NumEvents; ++i) {
                Counts[i] += oth.Counts[i];
            }
            return *this;
        }

    private:
        std::array<size_t, NumEvents> Counts;
    };

    template <size_t SizeInKb,
              size_t NumSlots,
              size_t RetryCount>
    struct TBlobPage {
        std::array<char, SizeInKb << 10UL> Bytes;
        std::array<TIndexType, NumSlots> Slots;

        size_t UsedBytes = 0;
        size_t UsedSlots = 0;

        TEventStats Stats;

        Y_FORCE_INLINE bool GetLock() {
            return NPagedBlobHasherPrivate::GetLock<RetryCount>(PageLock);
        }

        Y_FORCE_INLINE bool GetLockLazy() {
            return NPagedBlobHasherPrivate::GetLock<1>(PageLock);
        }

        Y_FORCE_INLINE bool IsLocked() {
            return NPagedBlobHasherPrivate::IsLocked(PageLock);
        }

        Y_FORCE_INLINE void ReleaseLock() {
            Y_ASSERT(UsedBytes <= Bytes.size());
            Y_ASSERT(UsedSlots <= Slots.size());
            NPagedBlobHasherPrivate::ReleaseLock(PageLock);
        }

        Y_FORCE_INLINE size_t AvailBytes() {
            return Bytes.size() - UsedBytes;
        }

        Y_FORCE_INLINE size_t AvailSlots() {
            return Slots.size() - UsedSlots;
        }

        Y_FORCE_INLINE size_t Avail() {
            return AvailSlots() > 0 ? AvailBytes() : 0;
        }

        Y_FORCE_INLINE bool Empty() {
            return UsedBytes == 0 && UsedSlots == 0;
        }

        Y_FORCE_INLINE char* AllocSlot(size_t size, TIndexType pairIndex) {
            Y_ASSERT(size > 0);

            if (size > Avail()) {
                return nullptr;
            }

            Y_ASSERT(UsedBytes < Bytes.size());
            Y_ASSERT(UsedSlots < Slots.size());

            char* ptr = &Bytes[0] + UsedBytes;
            SetIndexToSlot(Slots[UsedSlots], pairIndex);
            UsedBytes += size;
            UsedSlots += 1;

            return ptr;
        }

        Y_FORCE_INLINE void RollbackAlloc(size_t size, TIndexType pairIndex) {
            Y_ASSERT(size <= UsedBytes);
            Y_ASSERT(1 <= UsedSlots);
            Y_ASSERT(GetIndexFromSlot(Slots[UsedSlots - 1]) == pairIndex);

            UsedSlots -= 1;
            UsedBytes -= size;
        }

        Y_FORCE_INLINE static void SetIndexToSlot(TIndexType& slot, TIndexType pairIndex) {
            Y_ASSERT(pairIndex <= (MaxIndex >> 1UL));
            slot = (pairIndex << 1UL) | 1UL;
        }

        Y_FORCE_INLINE static bool IsValidSlot(TIndexType slot) {
            return (slot & 1UL) == 1UL;
        }

        Y_FORCE_INLINE static TIndexType GetIndexFromSlot(TIndexType slot) {
            Y_ASSERT(IsValidSlot(slot));
            return slot >> 1UL;
        }

    private:
        TAtomic PageLock = 0;
    };

}

template <typename TKey,
          typename TKeyCopier = NPagedBlobHasherDefaults::TNoOpKeyCopier<TKey>,
          typename TKeyEq = TEqualTo<TKey>,
          typename TKeyHash = THash<TKey>,
          typename TTraits = NPagedBlobHasherDefaults::TBlobHasherTraits>
class TPagedBlobHasher: public TTraits, public IPagedBlobHasher<TKey> {
private:
    friend class TPage;

    using THashPair = NPagedBlobHasherPrivate::THashPair<TKey, TTraits::RetryCount>;
    using TBlobPage = NPagedBlobHasherPrivate::TBlobPage<TTraits::PageSizeInKb,
                                                         TTraits::NumPageSlots,
                                                         TTraits::RetryCount>;

    using TPairs = std::array<THashPair, TTraits::NumPairs>;
    using TPages = std::array<TBlobPage, TTraits::NumPages>;

    using TEventStats = NPagedBlobHasherPrivate::TEventStats;
    using TParent = IPagedBlobHasher<TKey>;
    using IPage = typename TParent::IPage;
    using TPageUniqPtr = typename TParent::TPageUniqPtr;
    using IPagesHolder = typename TParent::IPagesHolder;
    using TPagesHolderUniqPtr = typename TParent::TPagesHolderUniqPtr;

public:
    using TIndexType = NPagedBlobHasherPrivate::TIndexType;
    using THashType = NPagedBlobHasherPrivate::THashType;

    static const TIndexType MaxIndex = NPagedBlobHasherPrivate::MaxIndex;

    static_assert(TTraits::NumPairs > 0, "num pairs should be > 0");
    static_assert(TTraits::NumPages > 0, "num pages should be > 0");
    static_assert(TTraits::NumPageSlots > 0, "num page slots should be > 0");
    static_assert(TTraits::PageSizeInKb > 0, "page size should be > 0");

    static_assert(TTraits::NumPairs <= (MaxIndex >> 1) + 1, "too many pairs");
    static_assert(TTraits::NumPages <= MaxIndex, "too many pages");
    static_assert(TTraits::NumPageSlots <= MaxIndex, "too many slots");

    Y_FORCE_INLINE static TIndexType Mod(TIndexType pageIndex) {
        return pageIndex % TTraits::NumPages;
    }

    // Single-threaded access to hasher.
    // Holds one or more preallocated
    // pages and automatically switches between them.
    // TPage objects can't be shared safely
    // by multiple threads.
    class TPage: public TNonCopyable, public IPage {
    private:
        friend class TPagedBlobHasher;

        TPage(TPagedBlobHasher* parent, TIndexType firstPage, size_t numPages)
            : Parent(parent)
            , FirstPage(firstPage)
            , NumPages(numPages)
            , CurPageOffset(0)
        {
            Y_ASSERT(Parent);
            Y_ASSERT(NumPages > 0);
        }

    public:
        ~TPage() override {
            for (TIndexType pageOfs = 0; pageOfs != NumPages; ++pageOfs) {
                Parent->ReleasePage(Mod(FirstPage + pageOfs));
            }
        }

        bool GetBlobOrFail(const TKey& key, TBlob& value) override {
            if (Parent->GetBlobOrFail(key, value, CurPage())) {
                UpdateCurrentPage(key, value);
                return true;
            }
            return false;
        }

        bool StoreBlobOrFail(const TKey& key, const TBlob& value) override {
            UpdateCurrentPage(key, value);
            return Parent->StoreBlobOrFail(key, value, CurPage());
        }

        size_t Avail() override {
            return Parent->GetPageAvail(CurPage()) +
                   (NumPages - CurPageOffset - 1) * (TTraits::PageSizeInKb << 10);
        }

        bool HasEnoughSpace(const TKey& key, const TBlob& value) override {
            TKeyCopier copier;
            return Avail() >= copier.GetSize(key) + value.Size();
        }

    private:
        Y_FORCE_INLINE TIndexType CurPage() const {
            return Mod(FirstPage + CurPageOffset);
        }

        void UpdateCurrentPage(const TKey& key, const TBlob& value) {
            TKeyCopier copier;
            const size_t totalSize = copier.GetSize(key) + value.Size();

            if (Parent->GetPageAvail(CurPage()) < totalSize) {
                if (CurPageOffset + 1 < NumPages) {
                    CurPageOffset += 1;
                }
            }
        }

    private:
        TPagedBlobHasher* Parent = nullptr;
        TIndexType FirstPage = MaxIndex;
        size_t NumPages = 0;
        TIndexType CurPageOffset = MaxIndex;
    };

    // Holds array of allocated pages and automatically
    // (though not perfectly) allocates new pages
    // until AllocPage fails or max number of pages
    // is reached.
    class TPagesHolder: public IPagesHolder {
    private:
        friend class TPagedBlobHasher;

        TPagesHolder(TPagedBlobHasher* parent, size_t maxPages)
            : Parent(parent)
            , MaxPages(maxPages)
        {
            Y_ASSERT(Parent);
            Y_ASSERT(MaxPages > 0);

            Pages.reserve(MaxPages); // never reallocs
            AllocNext();
        }

    public:
        bool GetBlobOrFail(const TKey& key, TBlob& value) override {
            if (!CurPage) {
                return false;
            }

            bool res = CurPage->GetBlobOrFail(key, value);

            if (!CurPage->HasEnoughSpace(key, value)) { // can't check before GetBlobOrFail
                AllocNext();
            }

            return res;
        }

        bool StoreBlobOrFail(const TKey& key, const TBlob& value) override {
            if (!CurPage) {
                return false;
            }

            bool res = CurPage->StoreBlobOrFail(key, value);

            if (!CurPage->HasEnoughSpace(key, value)) {
                AllocNext();
            }
            return res;
        }

    private:
        Y_FORCE_INLINE void AllocNext() {
            Y_ASSERT(Pages.size() <= MaxPages);
            Y_ASSERT(Pages.size() == 0 || Pages.back());

            if (Pages.size() >= MaxPages) {
                CurPage = nullptr;
                return;
            }

            Pages.push_back(Parent->AllocPage());
            CurPage = static_cast<TPage*>(Pages.back().Get());
        }

    private:
        TPagedBlobHasher* Parent = nullptr;
        size_t MaxPages = 0;
        TVector<TPageUniqPtr> Pages;
        TPage* CurPage = nullptr;
    };

public:
    TPageUniqPtr AllocPage(size_t num = 1) override {
        AcquireLock();
        const TIndexType firstPage = AllocUnsafe(num);
        ReleaseLock();

        if (firstPage != MaxIndex) {
            return new TPage(this, firstPage, num);
        } else {
            Stats.Inc(TEventStats::PageFail);
            return nullptr;
        }
    }

    TPagesHolderUniqPtr AllocPagesHolder(size_t maxPages) override {
        return new TPagesHolder(this, maxPages);
    }

    void CalcStats(THasherStats& stats) override {
        AcquireLock();
        CalcStatsUnsafe(stats);
        ReleaseLock();
    }

    size_t GetSize() override {
        return sizeof(*this);
    }

    void Suspend() override {
        AcquireLock();
        Enabled = false;
        ReleaseLock();
    }

    void Resume() override {
        AcquireLock();
        Enabled = true;
        ReleaseLock();
    }

private:
    bool StoreBlobOrFail(const TKey& key, const TBlob& value, TIndexType pageIndex) {
        Y_ASSERT(pageIndex < Pages.size());

        size_t hash = 0;
        THashPair& hashPair = GetHashPair(key, hash);
        TBlobPage& page = Pages[pageIndex];

        if (hashPair.GetLock()) {
            bool res = AccessBlobUnsafe<false>(GetIndex(hash), key, value, pageIndex);
            hashPair.ReleaseLock();
            return res;
        } else {
            page.Stats.Inc(TEventStats::StoreMiss);
        }

        return false;
    }

    bool GetBlobOrFail(const TKey& key, TBlob& value, TIndexType pageIndex) {
        Y_ASSERT(pageIndex < Pages.size());

        size_t hash = 0;
        TBlobPage& page = Pages[pageIndex];
        THashPair& hashPair = GetHashPair(key, hash);
        TKeyEq equalTo;

        Y_ASSERT(page.IsLocked());

        if (hashPair.GetLock()) {
            if (hashPair.IsValid() && equalTo(hashPair.Key, key)) {
                if (hashPair.GetPageIndex() == pageIndex) { // page flush won't happen
                    value = hashPair.Value;
                    hashPair.ReleaseLock();
                    page.Stats.Inc(TEventStats::Hit);
                    return true;
                }

                TBlob savedValue;
                if (!AccessBlobUnsafe<true>(GetIndex(hash), hashPair.Key, hashPair.Value, pageIndex, &savedValue)) {
                    hashPair.ReleaseLock();
                    return false; // couldn't get data - page flush
                }

                value = savedValue;
                hashPair.ReleaseLock();
                page.Stats.Inc(TEventStats::Hit);
                return true;
            } else {
                if (hashPair.WasValid) {
                    page.Stats.Inc(TEventStats::MissHot);
                } else {
                    page.Stats.Inc(TEventStats::MissCold);
                }
            }

            hashPair.ReleaseLock();
        } else {
            page.Stats.Inc(TEventStats::MissLock);
        }

        return false;
    }

    size_t GetPageAvail(TIndexType pageIndex) {
        Y_ASSERT(pageIndex < Pages.size());
        Y_ASSERT(Pages[pageIndex].IsLocked());
        return Pages[pageIndex].Avail();
    }

    void ReleasePage(TIndexType pageIndex) {
        Y_ASSERT(pageIndex < Pages.size());
        Pages[pageIndex].ReleaseLock();
    }

private:
    Y_FORCE_INLINE void AcquireLock() {
        NPagedBlobHasherPrivate::AcquireLock(HasherLock);
    }

    Y_FORCE_INLINE bool IsLocked() {
        return NPagedBlobHasherPrivate::IsLocked(HasherLock);
    }

    Y_FORCE_INLINE void ReleaseLock() {
        NPagedBlobHasherPrivate::ReleaseLock(HasherLock);
    }

    Y_FORCE_INLINE static THashType GetHash(const TKey& key) {
        TKeyHash hash;
        return hash(key);
    }

    Y_FORCE_INLINE static TIndexType GetIndex(THashType hash) {
        return hash % TTraits::NumPairs;
    }

    Y_FORCE_INLINE THashPair& GetHashPair(const TKey& key) {
        return Hash[GetIndex(GetHash(key))];
    }

    Y_FORCE_INLINE THashPair& GetHashPair(const TKey& key, THashType& hash) {
        hash = GetHash(key);
        return Hash[GetIndex(hash)];
    }

    TIndexType AllocUnsafe(size_t num = 1) {
        Y_ASSERT(IsLocked());

        if (!Enabled) {
            return MaxIndex;
        }

        const TIndexType firstPage = Head;
        const size_t numPages = num;

        Head = Mod(Head + numPages);

        bool failToLockPages = false;
        TIndexType numLockedPages = 0;
        for (TIndexType pageOfs = 0; pageOfs != numPages; ++pageOfs) {
            if (!Pages[Mod(firstPage + pageOfs)].GetLock()) { // no contention unless cache overflows
                failToLockPages = true;
                numLockedPages = pageOfs;
                break;
            }
        }

        if (failToLockPages) {
            for (TIndexType pageOfs = 0; pageOfs != numLockedPages; ++pageOfs) {
                Pages[Mod(firstPage + pageOfs)].ReleaseLock();
            }
            return MaxIndex;
        }

        for (TIndexType pageOfs = 0; pageOfs != numPages; ++pageOfs) {
            ClearPageUnsafe(Mod(firstPage + pageOfs));
        }
        return firstPage;
    }

    void ClearPageUnsafe(TIndexType pageIndex) {
        Y_ASSERT(pageIndex < Pages.size());

        TBlobPage& page = Pages[pageIndex];

        Y_ASSERT(IsLocked());
        Y_ASSERT(page.IsLocked());

        if (!page.Empty()) {
            Stats.Inc(TEventStats::PageFlush);
        }

        for (size_t i = 0; i != page.UsedSlots; ++i) {
            if (TBlobPage::IsValidSlot(page.Slots[i])) {
                TIndexType pairIndex = TBlobPage::GetIndexFromSlot(page.Slots[i]);
                Y_ASSERT(pairIndex < Hash.size());
                THashPair& hashPair = Hash[pairIndex];
                if (hashPair.HasPageIndex(pageIndex)) {
                    // No need to lock hashPair here, atomic write is enough
                    hashPair.Invalidate();
                }
            }
        }
        page.UsedBytes = 0;
        page.UsedSlots = 0;
    }

    // Somewhat ugly. Actually implements
    // both write (isRefresh=false) and
    // read with refresh (isRefresh=true)
    template <bool isRefresh>
    bool AccessBlobUnsafe(TIndexType pairIndex,
                          const TKey& key, const TBlob& value,
                          TIndexType pageIndex,
                          TBlob* savedValue = nullptr) {
        Y_ASSERT(pageIndex < Pages.size());
        Y_ASSERT(pairIndex < Hash.size());

        TBlobPage& page = Pages[pageIndex];
        THashPair& hashPair = Hash[pairIndex];
        TKeyCopier copier;

        Y_ASSERT(page.IsLocked());
        Y_ASSERT(hashPair.IsLocked());

        bool skipRefresh = false;
        if (isRefresh) {
            TIndexType srcPageIndex = hashPair.GetPageIndex();
            if (srcPageIndex != MaxIndex) {
                Y_ASSERT(srcPageIndex < Pages.size());
                TBlobPage& srcPage = Pages[srcPageIndex];
                if (srcPage.IsLocked()) { // do not realloc from locked page
                    skipRefresh = true;
                }
            }
        }

        const size_t keySize = copier.GetSize(key);
        const size_t valueSize = value.Size();
        if (page.Avail() >= keySize + valueSize && !skipRefresh) {
            char* buf = page.AllocSlot(keySize + valueSize, pairIndex);
            Y_ASSERT(buf);
            char* keyBuf = buf;
            char* valueBuf = keyBuf + keySize;

            memcpy(valueBuf, value.AsCharPtr(), valueSize);    // non-atomic, need to verify
            if (isRefresh && !hashPair.IsValid()) { // page flush happened in simultaneuous AllocPage
                page.RollbackAlloc(keySize + valueSize, pairIndex);
                page.Stats.Inc(TEventStats::FlushConflict);
                return false;
            }

            if (isRefresh) {
                page.Stats.Inc(TEventStats::Refresh);
            } else {
                page.Stats.Inc(TEventStats::Store);
            }
            hashPair.Key = copier.Realloc(key, keyBuf);
            hashPair.Value = TBlob::NoCopy(valueBuf, valueSize);
            hashPair.SetPageIndex(pageIndex);
            hashPair.WasValid = true;

            if (savedValue) {
                *savedValue = hashPair.Value;
            }
            return true;
        } else {
            if (skipRefresh) {
                page.Stats.Inc(TEventStats::SkipRefresh);
            } else {
                page.Stats.Inc(TEventStats::OverflowFail);
            }

            if (savedValue) {
                *savedValue = value.DeepCopy();         // non-atomic, need to verify
                if (isRefresh && !hashPair.IsValid()) { // page flush happened
                    page.Stats.Inc(TEventStats::FlushConflict);
                    return false;
                }
                return true;
            }
        }
        return false;
    }

    void CalcStatsUnsafe(THasherStats& stats) {
        Y_ASSERT(IsLocked());

        const size_t totalBytes = Pages[0].Bytes.size() * Pages.size();
        const size_t totalSlots = Pages[0].Slots.size() * Pages.size();

        const size_t hasherSize = GetSize();

        const size_t pairsSize = Hash.size() * sizeof(THashPair);
        const size_t pagesSize = Pages.size() * sizeof(TBlobPage);
        const size_t slotsSize = totalSlots * sizeof(TIndexType);
        const size_t bytesSize = totalBytes; // * sizeof(char)

        size_t totalAvailBytes = 0;
        size_t totalAvailSlots = 0;
        size_t totalBytesWaste = 0;
        size_t totalSlotsWaste = 0;
        size_t numSkippedPages = 0;

        TEventStats acc = Stats;

        for (TBlobPage& page : Pages) {
            if (!page.GetLockLazy()) {
                numSkippedPages += 1;
                continue;
            }

            acc += page.Stats;

            totalAvailBytes += page.AvailBytes();
            totalAvailSlots += page.AvailSlots();

            if (page.AvailSlots() == 0) {
                totalBytesWaste += page.AvailBytes();
            }

            if (page.AvailBytes() == 0) {
                totalSlotsWaste += page.AvailSlots();
            }

            page.ReleaseLock();
        }

        stats.NumMissLock = acc.Get(TEventStats::MissLock);
        stats.NumMissCold = acc.Get(TEventStats::MissCold);
        stats.NumMissHot = acc.Get(TEventStats::MissHot);
        stats.NumHit = acc.Get(TEventStats::Hit);
        stats.NumStore = acc.Get(TEventStats::Store);
        stats.NumStoreMiss = acc.Get(TEventStats::StoreMiss);
        stats.NumRefresh = acc.Get(TEventStats::Refresh);
        stats.NumSkipRefresh = acc.Get(TEventStats::SkipRefresh);
        stats.NumOverflowFail = acc.Get(TEventStats::OverflowFail);
        stats.NumFlushConflict = acc.Get(TEventStats::FlushConflict);
        stats.NumPageFlush = acc.Get(TEventStats::PageFlush);
        stats.NumPageFail = acc.Get(TEventStats::PageFail);

        stats.HasherSize = hasherSize;
        stats.PairsSizeFrac = (float)pairsSize / (float)hasherSize;
        stats.PagesSizeFrac = (float)pagesSize / (float)hasherSize;
        stats.BytesSizeFrac = (float)bytesSize / (float)hasherSize;
        stats.SlotsSizeFrac = (float)slotsSize / (float)hasherSize;

        stats.BytesUtilization = 1.f - (float)totalAvailBytes / (float)totalBytes;
        stats.SlotsUtilization = 1.f - (float)totalAvailSlots / (float)totalSlots;
        stats.BytesWaste = (float)totalBytesWaste / (float)totalBytes;
        stats.SlotsWaste = (float)totalSlotsWaste / (float)totalSlots;

        stats.NumSkippedPages = numSkippedPages;
    }

private:
    TPairs Hash;
    TPages Pages;
    TIndexType Head = 0;
    bool Enabled = true;

    TEventStats Stats;

    TAtomic HasherLock = 0;
};
