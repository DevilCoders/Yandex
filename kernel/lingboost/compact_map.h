#pragma once

#include "direct_map.h"

namespace NLingBoost {
    namespace NDetail {
        // Compact remapping of enum values
        template <typename MapType>
        class TCompactRemapBase {
        public:
            using TMapType = MapType;
            using TKeyIterator = typename TMapType::TKeyIterator;
            using TKey = typename TMapType::TKey;
            using TIndexRemap = typename TMapType::TIndexRemap;

            using TConstView = TCompactRemapBase<TDirectMapView<TKeyIterator, const i32>>;

        private:
            template <typename X>
            friend class TCompactRemapBase;

            TMapType Map;

            TCompactRemapBase(const TMapType& map)
                : Map(map)
            {}

        public:
            TCompactRemapBase() = default;
            template <typename... Args>
            TCompactRemapBase(TKeyIterator begin,
                TKeyIterator end,
                const TIndexRemap& indexRemap,
                Args&&... args)
                : Map(begin, end, indexRemap, Max<i32>(), std::forward<Args>(args)...)
            {
                i32 index = 0;
                for (auto iter : xrange(begin, end)) {
                    Map[*iter] = index;
                    ++index;
                }
            }
            template <typename X>
            TCompactRemapBase(const TCompactRemapBase<X>& other)
                : Map(other.Map)
            {}

            bool HasKey(TKey key) const {
                return Map.IsKeyInRange(key) && Map[key] != Max<i32>();
            }
            bool HasIndex(i32 index) const {
                return index >= 0 && static_cast<size_t>(index) < Map.Size();
            }

            TMapIndex GetIndex(TKey key) const {
                Y_ASSERT(HasKey(key));
                return TMapIndex(Map[key]);
            }
            TKey GetKey(i32 index) const {
                Y_ASSERT(HasIndex(index));
                return *(Map.KeysBegin() + index);
            }

            TKeyIterator KeysBegin() const {
                return Map.KeysBegin();
            }
            TKeyIterator KeysEnd() const {
                return Map.KeysEnd();
            }
            TArrayRef<TKey> GetKeys() const {
                return Map.GetKeys();
            }

            size_t Size() const {
                return Map.Size();
            }

            TConstView View() const {
                return TConstView(Map.View());
            }
        };
    } // NDetail

    template <typename KeyIterType, size_t Capacity>
    using TStaticCompactRemap = NDetail::TCompactRemapBase<TStaticDirectMap<KeyIterType, i32, Capacity>>;

    template <typename KeyIterType, typename AllocType = std::allocator<i32>>
    using TDynamicCompactRemap = NDetail::TCompactRemapBase<TDynamicDirectMap<KeyIterType, i32, AllocType>>;

    template <typename KeyIterType, typename AllocType = std::allocator<i32>>
    using TCompactRemap = TDynamicCompactRemap<KeyIterType, AllocType>;

    template <typename KeyIterType, typename AllocType = TPoolAlloc<i32>>
    class TPoolableCompactRemap
        : public NDetail::TCompactRemapBase<TPoolableDirectMap<KeyIterType, i32, AllocType>>
    {
    public:
        using TBase = NDetail::TCompactRemapBase<TPoolableDirectMap<KeyIterType, i32, AllocType>>;

        TPoolableCompactRemap() = default;

        template <typename... Args>
        TPoolableCompactRemap(typename TBase::TKeyIterator begin,
            typename TBase::TKeyIterator end,
            const typename TBase::TIndexRemap& indexRemap,
            Args&&... args)
            : TBase(begin, end, indexRemap, std::forward<Args>(args)...)
        {}

        template <typename PoolType, typename... Args>
        TPoolableCompactRemap(PoolType& pool, Args&&... args)
            : TBase(std::forward<Args>(args)..., AllocType(&pool))
        {}
    };

    template <typename KeyIterType>
    using TCompactRemapView = NDetail::TCompactRemapBase<TDirectMapView<KeyIterType, const i32>>;

    namespace NDetail {
        template <typename KeyIterType>
        struct TCompactRemapFunc {
            using TKeyIterator = KeyIterType;
            using TRemap = TCompactRemapView<TKeyIterator>;
            using TKey = typename TRemap::TKey;

            TRemap Remap;

            TCompactRemapFunc() = default;
            TCompactRemapFunc(const TRemap& remap) // implicit is ok
                : Remap(remap)
            {
            }

            Y_FORCE_INLINE i32 operator() (TKey value) const {
                return Remap.GetIndex(value);
            }
            Y_FORCE_INLINE bool IsInRange(TKey value) const {
                return Remap.HasKey(value);
            }
        };

        template <typename KeyIterType,
            typename StorageType>
        using TCompactMapBase = TDirectMapBase<KeyIterType,
            TCompactRemapFunc<KeyIterType>,
            StorageType>;
    } // NDetail

    template <typename KeyIterType,
        typename ValueType,
        size_t Capacity>
    using TStaticCompactMap = NDetail::TCompactMapBase<KeyIterType,
        NDetail::TStaticMapStorage<ValueType, Capacity>>;

    template <typename KeyIterType,
        typename ValueType,
        typename AllocType = std::allocator<ValueType>>
    using TDynamicCompactMap = NDetail::TCompactMapBase<KeyIterType,
        NDetail::TDynamicMapStorage<ValueType, AllocType>>;

    template <typename KeyIterType,
        typename ValueType,
        typename AllocType = std::allocator<ValueType>>
    using TCompactMap = TDynamicCompactMap<KeyIterType, ValueType, AllocType>;

    template <typename KeyIterType,
        typename ValueType,
        typename AllocType = TPoolAlloc<ValueType>>
    class TPoolableCompactMap
        : public NDetail::TCompactMapBase<KeyIterType,
            NDetail::TPoolableMapStorage<ValueType, AllocType>>
    {
    public:
        using TBase = NDetail::TCompactMapBase<KeyIterType,
            NDetail::TPoolableMapStorage<ValueType, AllocType>>;

        TPoolableCompactMap() = default;

        template <typename... Args>
        TPoolableCompactMap(typename TBase::TKeyIterator begin,
            typename TBase::TKeyIterator end,
            const typename TBase::TIndexRemap& indexRemap,
            typename TBase::TValueParam fillValue,
            Args&&... args)
            : TBase(begin, end, indexRemap, fillValue, std::forward<Args>(args)...)
        {}

        template <typename PoolType, typename... Args>
        TPoolableCompactMap(PoolType& pool, Args&&... args)
            : TBase(std::forward<Args>(args)..., AllocType(&pool))
        {}
    };

    template <typename KeyIterType,
        typename ValueType>
    using TCompactMapView = NDetail::TCompactMapBase<KeyIterType,
        NDetail::TViewMapStorage<ValueType>>;
} // NLingBoost
