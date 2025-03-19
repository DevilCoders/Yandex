#pragma once

/*
The RCU (read-copy-update synchronization) hash relies on the fact that TKey value can
be set atomically. Also, it is assumed that reading and writing threads have different
instances of the TRCUHash object. Writing thread can be only one. Any resizing or other
unsafe operations are made only as copy-on-write assuming that reference counter equal to
one means that no one reading thread is using this instance. Adding without resizing is made atomic.
*/

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/ymath.h>
#include <utility>

template <class TKey, class TValue, class TEmptyKey, class THashFunct = THash<TKey> >
class TRCUHashBase {
public:
    typedef std::pair<TKey, TValue> TKeyValue;

    class const_iterator {
        typedef TVector<TKeyValue> TContainer;
        typedef typename TContainer::const_iterator TBase;
        TBase Iterator;
        const TContainer& Container;
    public:
        // for with "begin" iterator initialization
        const_iterator (const TContainer& container, TBase beg, bool isBegin)
            : Iterator(beg)
            , Container(container)
        {
            if (isBegin)
                while(Iterator != Container.end() && Iterator->first == TEmptyKey())
                    ++Iterator;
        }

        void operator++ () {
            if (Iterator != Container.end())
                ++Iterator;

            while(Iterator != Container.end() && Iterator->first == TEmptyKey())
                ++Iterator;
        }

        bool operator!= (const_iterator other) {
            return other.Iterator != Iterator;
        }

        TBase operator->() {
            return Iterator;
        }

        const TKeyValue& operator*() const {
            return *Iterator;
        }
    };

    typedef THashMap<TKey, size_t> TKeysCounts;
private:
    TVector<TKeyValue> Container;
    THashFunct HashFunct;

    static inline size_t UpSize(size_t n) {
        return n ? ::FastClp2<size_t>(n) : 1;
    }

public:
    typedef TKeyValue* TIterator;
    typedef TKeyValue const* TConstIterator;

    TRCUHashBase(size_t userCapacity = 0) {
        ClearAndResize(userCapacity);
    }

    TRCUHashBase(TRCUHashBase& src, TKeysCounts* keysCounts, size_t userCapacity) {
        ClearAndResize(userCapacity);
        CopyStorage(src, keysCounts);
    }

    TIterator Find(const TKey& key) {
        TIterator res = nullptr;

        size_t hashOffset;
        if (FindHashedOffset(key, hashOffset))
            res = &Container[hashOffset];

        return res;
    }

    TConstIterator Find(const TKey& key) const {
        const TIterator res = NULL;

        size_t hashOffset;
        if (FindHashedOffset(key, hashOffset))
            res = &Container[hashOffset];

        return res;
    };

    // Capacity - size of buffer
    inline size_t GetCapacity() const {
        return Container.size();
    }

    // TODO need change for better using (use in PrintContainer but don't work properly)
    const TKeyValue& operator[] (size_t index) const {
        return Container[index];
    }

    TKeyValue& operator[] (size_t index) {
        return Container[index];
    }

    const_iterator begin() const {
        return const_iterator(Container, Container.begin(), true);
    }

    const_iterator end() const {
        return const_iterator(Container, Container.end(), false);
    }

    inline TKeyValue* AddInternal(const TKey& key, const TValue& value) {

        size_t hashOffset = GetHashOffset(key);
        size_t capacity = GetCapacity();
        static_assert(sizeof(TKey) <= sizeof(void*), "expect sizeof(TKey) <= sizeof(void*)");
        Y_ASSERT(hashOffset < capacity);

        // find first empty key or equal key
        while (Container[hashOffset].first != key && Container[hashOffset].first != TEmptyKey() )
            IncHashOffset(hashOffset);

        Container[hashOffset].second = value;
        ATOMIC_COMPILER_BARRIER();
        Container[hashOffset].first = key;        // this operation is atomic, because key is filled last

        return &Container[hashOffset];
    }

    void ClearAndResize(size_t newCapacity) {
        // in "resize", keys of all filled elements become undefined.
        Container.clear();
        Container.resize(UpSize(newCapacity), TKeyValue(TEmptyKey(), TValue()));
    }
private:
    inline void IncHashOffset(size_t& hashOffset) {
        hashOffset++;
        hashOffset = hashOffset & (Container.size() - 1);
    }

    bool FindHashedOffset(const TKey& key, size_t& hashOffset) {
        hashOffset = GetHashOffset(key);
        Y_ASSERT(hashOffset < Container.size());

        // skip filled cells with wrong keys.
        size_t cycleCount = Container.size();         // to avoid cycling
        while (Container[hashOffset].first != key && Container[hashOffset].first != TEmptyKey() && cycleCount--)
            IncHashOffset(hashOffset);

        return Container[hashOffset].first == key;
    }

    inline size_t GetHashOffset(const TKey& key) const {
        return HashFunct(key) & (Container.size() - 1);
    }

    void CopyStorage(TRCUHashBase& src, TKeysCounts* keysCounts) {
        if (keysCounts) {
            for (size_t i = 0; i < src.GetCapacity(); ++i) {
                // check empty keys in hash array
                if (src[i].first != TEmptyKey()) {
                    typename TKeysCounts::iterator it = keysCounts->find(src[i].first); // check key counts
                    if (it != keysCounts->end() && !it->second) {                  // if zero count remove from key counts
                        keysCounts->erase(it);                                     // not moved in new hash version
                        continue;
                    }
                    AddInternal(src[i].first, src[i].second);
                }
            }
        } else {
            for (size_t i = 0; i < src.GetCapacity(); ++i)
                if (src[i].first != TEmptyKey())
                    AddInternal(src[i].first, src[i].second);
        }
    }
};

template <class TKey, class TValue, class TEmptyKey, class THashFunct = THash<TKey> >
class TRCUHash : public TSimpleSharedPtr<TRCUHashBase<TKey, TValue, TEmptyKey, THashFunct> > {
public:
    typedef TRCUHashBase<TKey, TValue, TEmptyKey, THashFunct> TRCUHashImpl;
    typedef typename TRCUHashImpl::TKeysCounts TKeysCounts;

private:
    size_t Size;
    TKey MaxKey;
    size_t DeletionCounter;
    TKeysCounts* KeysCounts;
    float ResizeFactor;
    size_t MaxBusyElements;
    size_t MinFreeElements;
public:
    typedef TSimpleSharedPtr<TRCUHashImpl> TBase;
    typedef typename TRCUHashImpl::TKeyValue TKeyValue;

    // for STL compatibility
    typedef TKey key_type;
    typedef TValue mapped_type;
    typedef THashFunct hashed_type;

    typedef typename TRCUHashImpl::const_iterator const_iterator;

    const_iterator begin() const {
        return  (**this).begin();
    }

    const_iterator end() const {
        return  (**this).end();
    }

    static const size_t DefaultHashTableSize = 128;

    TRCUHash(size_t userCapacity = DefaultHashTableSize, float resizeFactor = 0.1)
        : TBase(new TRCUHashImpl(userCapacity))
        , Size(0)
        , MaxKey(TKey())
        , DeletionCounter(0)
        , KeysCounts(nullptr)
        , ResizeFactor(resizeFactor)
    {
        Y_VERIFY(ResizeFactor > 0 && ResizeFactor <= 1, "ResizeFactor must be in (0..1] range, now %f", ResizeFactor);
        RecalcSpaceBorders();
    }

    void SetKeysCounts(TKeysCounts* keysCounts) {
        KeysCounts = keysCounts;
    }

    TKeyValue* Add(const TKey& key, const TValue& value) {
        if (key > MaxKey)
            MaxKey = key;

        size_t capacity = (**this).GetCapacity();

        size_t busySpace = Size + DeletionCounter;
        size_t freeSpace = capacity - busySpace;
        if (busySpace > 2 * MaxBusyElements || freeSpace <= MinFreeElements / 2) {
            ResizeInternal(capacity * 2);
        }

        TKeyValue* res = (**this).AddInternal(key, value);
        ++Size;

        return res;
    }

    void Remove(const TKey& key) {
        TKeyValue* it = NULL;
        if (it = (**this).Find(key)) {
            DeletionCounter++;

            // not used Ref count from sharedPtr
            if (DeletionCounter > (**this).GetCapacity() / 10)  // resize if Deletion counter more than 1/10 capacity
                ResizeInternal(Size * 10);

            --Size;
        }
    }

    TKeyValue* Find(const TKey& key) const {
        return (**this).Find(key);
    }

    TKey GetMaxKey() const {
        return MaxKey;
    }

    size_t operator+ () const {
        return Size;
    }

    TValue& operator[] (const TKey& key) {
        TKeyValue* keyValue = (**this).Find(key);
        if (!keyValue)
            keyValue = Add(key, TValue());

        return keyValue->second;
    }

    // for base search, to avoid reallocation.
    void ClearAndResize(size_t newCapacity) {
        (**this).ClearAndResize(newCapacity);
        Size = 0;
        MaxKey = TKey();
        DeletionCounter = 0;

        if (KeysCounts)
            KeysCounts->clear();

        // call RecalcSpaceBorders last, DeletionCounter are used.
        RecalcSpaceBorders();
    }
private:
    void ResizeInternal(size_t capacity) {
        TBase newStorage(new TRCUHashImpl(**this, KeysCounts, capacity));
        TBase::Swap(newStorage);       // swap must be "atomic"
        DeletionCounter = 0;
        RecalcSpaceBorders();
    }

    inline void RecalcSpaceBorders() {
        size_t capacity = (**this).GetCapacity();
        MaxBusyElements = size_t(ResizeFactor * (float)capacity);
        CheckBorders(MaxBusyElements, capacity);

        MinFreeElements = size_t((1 - ResizeFactor) * (float)capacity);
        CheckBorders(MinFreeElements, capacity);
    }

    static inline void CheckBorders(size_t& value, size_t capacity) {
        value = value >= 1 ? value : 1;
        value = value > capacity ? capacity : value;
    }
};

