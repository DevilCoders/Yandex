#pragma once

#include <util/memory/pool.h>
#include <util/generic/xrange.h>
#include <util/generic/array_ref.h>
#include <util/generic/algorithm.h>
#include <array>

// This header describes light weight map class that
//  * has fixed size and memory footprint
//  * has predefined set of keys (e.g. set of enum values)
//  * keys are mapped to integers [0;N), mapping is injective (i.e. no collisions)
//  * N is the number of buckets allocated by map
//
// Such maps are used by lingboost runtime to keep small(-ish) lookup tables
// with high access rates (e.g. several times per each text machine hit)
//
// See enum_map.h for map class where key is lingboost enum
// See compact_map.h for "perfect" maps (i.e. 1-1 mapping between keys and integers from [0;N))
//

namespace NLingBoost {
    // Raw index that
    // can be used to
    // access maps as
    // vectors.
    struct TMapIndex {
        i32 Value = 0;

        TMapIndex() = default;
        template <typename T>
        explicit TMapIndex(T value)
            : Value(value)
        {
            static_assert(std::is_integral<T>::value, "");
            Y_ASSERT(static_cast<size_t>(value) <= static_cast<size_t>(Max<i32>()));
            Y_ASSERT(static_cast<i64>(value) >= static_cast<i64>(Min<i32>()));
        }

        operator i32() const {
            return Value;
        }
    };

    template <typename KeyType, typename ValueType>
    class TMapEntry {
    public:
        using TKey = KeyType;
        using TValue = ValueType;

    public:
        TMapEntry() = default;
        TMapEntry(TKey key, TValue* value, i32 index)
            : Key_(key)
            , Value_(value)
            , Index_(index)
        {
            Y_ASSERT(Value_);
        }

        Y_FORCE_INLINE TKey Key() const {
            return Key_;
        }
        Y_FORCE_INLINE TValue& Value() const {
            return *Value_;
        }
        Y_FORCE_INLINE TMapIndex Index() const {
            return Index_;
        }

    private:
        TKey Key_;
        TValue* Value_ = nullptr;
        TMapIndex Index_;
    };

    namespace NDetail {
        // No allocation, shallow copy
        template <typename ValueType>
        class TViewMapStorage {
        public:
            using TValue = ValueType;

            template <typename X>
            friend class TViewMapStorage;

        public:
            TViewMapStorage() = default;
            TViewMapStorage(TValue* begin, size_t capacity)
                : Data(begin, capacity)
            {
                Y_ASSERT(!!begin || 0 == capacity);
            }

            template <typename StorageType>
            TViewMapStorage(const StorageType& other)
                : Data(other.Begin(), other.Size())
            {}
            template <typename StorageType>
            TViewMapStorage& operator = (const StorageType& other) {
                Data = TArrayRef<TValue>(other.Begin(), other.Size());
                return *this;
            }

            Y_FORCE_INLINE TValue* Begin() {
                return Data.begin();
            }
            Y_FORCE_INLINE const TValue* Begin() const {
                return Data.begin();
            }
            Y_FORCE_INLINE size_t Size() const {
                return Data.size();
            }

        private:
            TArrayRef<TValue> Data;
        };

        // Static allocation, deep copy
        template <typename ValueType, size_t Cap>
        class TStaticMapStorage {
        public:
            using TValue = ValueType;
            using TValueParam = typename TTypeTraits<TValue>::TFuncParam;

            enum {
                Capacity = Cap,
                MaxCapacity = 1024
            };

            static_assert(Capacity <= MaxCapacity, "");

        public:
            TStaticMapStorage() = default;
            TStaticMapStorage(TValueParam value) {
                Data.fill(value);
            }

            Y_FORCE_INLINE TValue* Begin() {
                return Data.begin();
            }
            Y_FORCE_INLINE const TValue* Begin() const {
                return Data.begin();
            }
            Y_FORCE_INLINE size_t Size() const {
                return Data.size();
            }

        private:
            std::array<TValue, Capacity> Data;
        };

        // Dynamic allocation, deep copy
        template <typename ValueType, typename AllocType>
        class TDynamicMapStorage {
        public:
            using TValue = ValueType;
            using TValueParam = typename TTypeTraits<TValue>::TFuncParam;
            using TAlloc = typename std::allocator_traits<AllocType>::template rebind_alloc<TValue>;

        public:
            TDynamicMapStorage() = default;
            TDynamicMapStorage(TValueParam value, size_t capacity, const TAlloc& alloc = TAlloc())
                : Alloc(alloc)
                , Data(Allocate(capacity))
            {
                Construct(Data, value);
            }
            ~TDynamicMapStorage() {
                Destroy(Data);
                Deallocate(Data);
            }

            TDynamicMapStorage(const TDynamicMapStorage& other)
                : Data(Allocate(other.Data.size()))
            {
                Construct(Data, other.Data);
            }
            TDynamicMapStorage& operator = (const TDynamicMapStorage& other) {
                if (Y_UNLIKELY(other.Data.data() == Data.data())) {
                    Y_ASSERT(other.Data.size() == Data.size());
                    return *this;
                }

                if (other.Data.size() != Data.size()) {
                    Destroy(Data);
                    Deallocate(Data);
                    try {
                        Data = Allocate(other.Data.size());
                        Construct(Data, other.Data);
                    } catch (...) {
                        Data = TArrayRef<TValue>();
                        throw;
                    }
                } else {
                    Copy(other.Data.begin(), other.Data.end(), Data.begin());
                }
                return *this;
            }

            Y_FORCE_INLINE const TValue* Begin() const {
                return Data.begin();
            }
            Y_FORCE_INLINE TValue* Begin() {
                return Data.begin();
            }
            Y_FORCE_INLINE size_t Size() const {
                return Data.size();
            }

        private:
            TArrayRef<TValue> Allocate(size_t capacity) {
                return TArrayRef<TValue>{Alloc.allocate(capacity), capacity};
            }
            void Construct(TArrayRef<TValue> data, TValueParam value) {
                for (size_t i : xrange(data.size())) {
                    ::new (data.begin() + i) TValue(value);
                }
            }
            void Construct(TArrayRef<TValue> data, TArrayRef<TValue> otherData) {
                Y_ASSERT(data.size() == otherData.size());
                for (size_t i : xrange(data.size())) {
                    ::new (data.begin() + i) TValue(otherData[i]);
                }
            }
            void Destroy(TArrayRef<TValue> data) {
                for (size_t i : xrange(data.size())) {
                    (data.begin()+i)->~TValue();
                }
            }
            void Deallocate(TArrayRef<TValue> data) {
                Alloc.deallocate(data.data(), data.size());
            }
        private:
            TAlloc Alloc;
            TArrayRef<TValue> Data;
        };

        // Pool allocation, shallow copy, no dtor
        template <typename ValueType, typename AllocType>
        class TPoolableMapStorage {
        public:
            using TValue = ValueType;
            using TValueParam = typename TTypeTraits<TValue>::TFuncParam;
            using TAlloc = typename AllocType::template rebind<TValue>::other;

        public:
            TPoolableMapStorage() = default;
            TPoolableMapStorage(size_t capacity, const TAlloc& alloc = TAlloc())
                : Data(TAlloc(alloc).allocate(capacity), capacity)
            {
            }
            TPoolableMapStorage(TValueParam value, size_t capacity, const TAlloc& alloc = TAlloc())
                : Data(TAlloc(alloc).allocate(capacity), capacity)
            {
                for (size_t i : xrange(Data.size())) {
                    TAlloc(alloc).construct(Data.begin() + i, value);
                }
            }

            TPoolableMapStorage(const TPoolableMapStorage& other)
                : Data(other.Data)
            {
            }
            TPoolableMapStorage& operator = (const TPoolableMapStorage& other) {
                Data = other.Data;
                return *this;
            }

            Y_FORCE_INLINE const TValue* Begin() const {
                return Data.begin();
            }
            Y_FORCE_INLINE TValue* Begin() {
                return Data.begin();
            }
            Y_FORCE_INLINE size_t Size() const {
                return Data.size();
            }

        private:
            TArrayRef<TValue> Data;
        };

        template <typename KeyType>
        struct TShiftIndexRemap {
            using TKey = KeyType;

            i32 Offset = 0;

            TShiftIndexRemap() = default;
            TShiftIndexRemap(i32 offset) // implicit is ok
                : Offset(offset)
            {}

            Y_FORCE_INLINE i32 operator()(TKey key) const {
                return static_cast<i32>(key) - Offset;
            }
            Y_FORCE_INLINE bool IsInRange(TKey /*key*/) const {
                return true;
            }
        };

        struct TDirectMapDontInitialize {};

        template <typename KeyIterType,
            typename IndexRemapType,
            typename StorageType>
        class TDirectMapBase {
        public:
            using TIndexRemap = IndexRemapType;
            using TKey = typename TIndexRemap::TKey;
            using TKeyIterator = KeyIterType;

            using TStorage = StorageType;
            using TValue = typename TStorage::TValue;
            using TConstValue = const std::remove_const_t<TValue>;
            using TValueParam = typename TTypeTraits<TValue>::TFuncParam;
            using TValueIterator = TValue*;
            using TConstValueIterator = TConstValue*;

            using TSelf = TDirectMapBase<TKeyIterator, TIndexRemap, TStorage>;
            using TView = TDirectMapBase<TKeyIterator, TIndexRemap, NDetail::TViewMapStorage<TValue>>;
            using TConstView = TDirectMapBase<TKeyIterator, TIndexRemap, NDetail::TViewMapStorage<TConstValue>>;

            using DontInitialize = TDirectMapDontInitialize;

        private:
            template <typename X, typename Y, typename Z>
            friend class TDirectMapBase;

            TKeyIterator Begin{};
            TKeyIterator End{};
            TIndexRemap IndexRemap;
            TStorage Storage;

        public:
            TDirectMapBase() = default;

            template <typename... Args>
            TDirectMapBase(TKeyIterator begin,
                TKeyIterator end,
                const TIndexRemap& indexRemap,
                TValueParam fillValue,
                Args&&... args)
                : Begin(begin)
                , End(end)
                , IndexRemap(indexRemap)
                , Storage(fillValue, std::forward<Args>(args)...)
            {
                Y_ASSERT(begin <= end);
                Y_ASSERT(ValidateDomain(begin, end));
            }
            template <typename... Args>
            TDirectMapBase(DontInitialize,
                TKeyIterator begin,
                TKeyIterator end,
                const TIndexRemap& indexRemap,
                Args&&... args)
                : Begin(begin)
                , End(end)
                , IndexRemap(indexRemap)
                , Storage(std::forward<Args>(args)...)
            {
                Y_ASSERT(begin <= end);
                Y_ASSERT(ValidateDomain(begin, end));
            }

            template <typename X, typename Y, typename Z>
            TDirectMapBase(const TDirectMapBase<X, Y, Z>& other)
                : Begin(other.Begin)
                , End(other.End)
                , IndexRemap(other.IndexRemap)
                , Storage(other.Storage)
            {}

            bool IsIndexInRange(i32 index) const {
                return index >= 0 && size_t(index) < Storage.Size();
            }
            bool IsKeyInRange(TKey key) const {
                return IndexRemap.IsInRange(key) && IsIndexInRange(IndexRemap(key));
            }
            TMapIndex GetIndex(TKey key) const {
                Y_ASSERT(IsKeyInRange(key));
                return TMapIndex(IndexRemap(key));
            }

            Y_FORCE_INLINE TConstValueIterator Ptr(TKey key) const {
                Y_ASSERT(Storage.Begin());
                Y_ASSERT(IsKeyInRange(key));
                return Storage.Begin() + IndexRemap(key);
            }
            Y_FORCE_INLINE TValueIterator Ptr(TKey key) {
                Y_ASSERT(Storage.Begin());
                Y_ASSERT(IsKeyInRange(key));
                return Storage.Begin() + IndexRemap(key);
            }
            Y_FORCE_INLINE TConstValueIterator PtrNoRemap(i32 index) const {
                Y_ASSERT(Storage.Begin());
                Y_ASSERT(IsIndexInRange(index));
                return Storage.Begin() + index;
            }
            Y_FORCE_INLINE TValueIterator PtrNoRemap(i32 index) {
                Y_ASSERT(Storage.Begin());
                Y_ASSERT(IsIndexInRange(index));
                return Storage.Begin() + index;
            }

            Y_FORCE_INLINE TValueParam GetNoRemap(i32 index) const {
                Y_ASSERT(Storage.Begin());
                Y_ASSERT(IsIndexInRange(index));
                return Storage.Begin()[index];
            }
            Y_FORCE_INLINE TValue& RefNoRemap(i32 index) {
                Y_ASSERT(Storage.Begin());
                Y_ASSERT(IsIndexInRange(index));
                return Storage.Begin()[index];
            }
            Y_FORCE_INLINE TValueParam Get(TKey key) const {
                Y_ASSERT(Storage.Begin());
                Y_ASSERT(IsKeyInRange(key));
                return Storage.Begin()[IndexRemap(key)];
            }
            Y_FORCE_INLINE TValue& Ref(TKey key) {
                Y_ASSERT(Storage.Begin());
                Y_ASSERT(IsKeyInRange(key));
                return Storage.Begin()[IndexRemap(key)];
            }

            Y_FORCE_INLINE TValueParam operator[](TKey key) const {
                return Get(key);
            }
            Y_FORCE_INLINE TValue& operator[](TKey key) {
                return Ref(key);
            }
            Y_FORCE_INLINE TValueParam operator[](TMapIndex index) const {
                return GetNoRemap(index);
            }
            Y_FORCE_INLINE TValue& operator[](TMapIndex index) {
                return RefNoRemap(index);
            }

            TKeyIterator KeysBegin() const {
                return Begin;
            }
            TKeyIterator KeysEnd() const {
                return End;
            }
            TArrayRef<TKey> GetKeys() const {
                return {Begin, static_cast<size_t>(End - Begin)};
            }

            void Fill(TValueParam value) {
                for (TKeyIterator iter : xrange(Begin, End)) {
                    Ref(*iter) = value;
                }
            }
            template <typename IterType>
            void Insert(IterType begin, IterType end) {
                for (auto iter : xrange(begin, end)) {
                    Ref(iter->first) = iter->second;
                }
            }
            void Insert(std::initializer_list<std::pair<TKey, TValue>> list) {
                Insert(list.begin(), list.end());
            }

            size_t Size() const {
                return static_cast<size_t>(End - Begin);
            }
            size_t Capacity() const {
                return Storage.Size();
            }

            TView View() {
                return TView(DontInitialize(), Begin, End, IndexRemap, Storage.Begin(), Storage.Size());
            }
            TConstView View() const {
                return TConstView(DontInitialize(), Begin, End, IndexRemap, Storage.Begin(), Storage.Size());
            }

        private:
            template <typename EntryType, typename ParentType>
            class TIteratorBase {
            public:
                using TEntry = EntryType;
                using TEntryValue = typename TEntry::TValue;
                using TParent = ParentType;

            public:
                TIteratorBase() = default;
                TIteratorBase(TKeyIterator curKey, TParent* parent)
                    : CurKey(curKey)
                    , Parent(parent)
                {
                }

                TEntry Get() const {
                    Y_ASSERT(Parent);
                    Y_ASSERT(CurKey < Parent->End);
                    const TKey key = *CurKey;
                    const i32 index = Parent->GetIndex(key);
                    return TEntry(key, Parent->PtrNoRemap(index), index);
                }
                void Next() {
                    ++CurKey;
                }

                TEntry operator*() const {
                    return Get();
                }
                // NOTE. Overloading "->" would require
                // keeping local copy of TEntry.
                // Can be done, but seems unnecessary.
                void operator++() {
                    Next();
                }
                void operator++(int) {
                    Next();
                }
                bool operator==(const TIteratorBase<TEntry, TParent>& other) const {
                    return CurKey == other.CurKey;
                }
                bool operator!=(const TIteratorBase<TEntry, TParent>& other) const {
                    return CurKey != other.CurKey;
                }

            private:
                TKeyIterator CurKey{};
                TParent* Parent = nullptr;
            };

        public:
            using TIterator = TIteratorBase<TMapEntry<TKey, TValue>, TSelf>;
            using TConstIterator = TIteratorBase<TMapEntry<TKey, TConstValue>, const TSelf>;

            TConstIterator begin() const {
                return TConstIterator(Begin, this);
            }
            TConstIterator end() const {
                return TConstIterator(End, this);
            }
            TIterator begin() {
                return TIterator(Begin, this);
            }
            TIterator end() {
                return TIterator(End, this);
            }

        private:
            bool ValidateDomain(TKeyIterator begin, TKeyIterator end) const {
                std::remove_const_t<TKey> prevKey{};
                for (auto iter : xrange(begin, end)) {
                    Y_ASSERT(IsKeyInRange(*iter));
                    Y_ASSERT(iter == begin || prevKey < *iter);
                    prevKey = *iter;
                }
                return true;
            }
        };
    } // NDetail

    template <typename KeyIterType,
        typename ValueType,
        size_t Capacity>
    using TStaticDirectMap = NDetail::TDirectMapBase<KeyIterType,
        NDetail::TShiftIndexRemap<std::remove_reference_t<decltype(*KeyIterType())>>,
        NDetail::TStaticMapStorage<ValueType, Capacity>>;

    template <typename KeyIterType,
        typename ValueType,
        typename AllocType = std::allocator<ValueType>>
    using TDynamicDirectMap = NDetail::TDirectMapBase<KeyIterType,
        NDetail::TShiftIndexRemap<std::remove_reference_t<decltype(*KeyIterType())>>,
        NDetail::TDynamicMapStorage<ValueType, AllocType>>;

    template <typename KeyIterType,
        typename ValueType,
        typename AllocType = std::allocator<ValueType>>
    using TDirectMap = TDynamicDirectMap<KeyIterType, ValueType, AllocType>;

    template <typename KeyIterType,
        typename ValueType,
        typename AllocType = TPoolAlloc<ValueType>>
    class TPoolableDirectMap
        : public NDetail::TDirectMapBase<KeyIterType,
            NDetail::TShiftIndexRemap<std::remove_reference_t<decltype(*KeyIterType())>>,
            NDetail::TPoolableMapStorage<ValueType, AllocType>>
    {
    public:
        using TBase = NDetail::TDirectMapBase<KeyIterType,
            NDetail::TShiftIndexRemap<std::remove_reference_t<decltype(*KeyIterType())>>,
            NDetail::TPoolableMapStorage<ValueType, AllocType>>;

        TPoolableDirectMap() = default;

        template <typename... Args>
        TPoolableDirectMap(typename TBase::TKeyIterator begin,
            typename TBase::TKeyIterator end,
            const typename TBase::TIndexRemap& indexRemap,
            typename TBase::TValueParam fillValue,
            Args&&... args)
            : TBase(begin, end, indexRemap, fillValue, std::forward<Args>(args)...)
        {}

        template <typename PoolType, typename... Args>
        TPoolableDirectMap(PoolType& pool, Args&&... args)
            : TBase(std::forward<Args>(args)..., AllocType(&pool))
        {}
    };

    template <typename KeyIterType,
        typename ValueType>
    using TDirectMapView = NDetail::TDirectMapBase<KeyIterType,
        NDetail::TShiftIndexRemap<std::remove_reference_t<decltype(*KeyIterType())>>,
        NDetail::TViewMapStorage<ValueType>>;
} // NLingBoost

