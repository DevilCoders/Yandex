#pragma once

#include "compact_map.h"
#include "direct_map.h"

#include <util/memory/pool.h>
#include <util/generic/array_ref.h>

namespace NLingBoost {

    //
    // Direct enum maps
    //

    namespace NDetail {
        template <typename EnumType>
        struct TDomainTraits {
            i32 Offset = 0;  // index of first (minimal) key
            size_t Size = 0; // how many indices fit in the domain, i.e. max - min + 1

            TDomainTraits() = default;
            TDomainTraits(const TArrayRef<const EnumType>& domain) {
                Y_ASSERT(domain.size() <= 1 || domain[domain.size() - 1] > domain[0]);
                if (domain.size() > 0) {
                    Offset = static_cast<i32>(domain[0]);
                    Size = static_cast<size_t>(domain[domain.size() - 1] - domain[0] + 1);
                }
            }
        };
    } // NDetail

    template <typename EnumStructType, typename ValueType>
    class TStaticEnumMap
        : public TStaticDirectMap<const typename EnumStructType::EType*, ValueType, EnumStructType::Size>
    {
    public:
        using TEnumStruct = EnumStructType;
        using TEnum = typename TEnumStruct::EType;
        using TValue = ValueType;
        using TValueParam = typename TTypeTraits<TValue>::TFuncParam;

        using TBase = TStaticDirectMap<const TEnum*, TValue, TEnumStruct::Size>;

        explicit TStaticEnumMap(TValueParam value = TValue())
            : TBase(TEnumStruct::GetValuesRegion().begin(),
                TEnumStruct::GetValuesRegion().end(),
                0, value)
        {}
    };

    template <typename EnumStructType, typename ValueType, typename AllocType = std::allocator<ValueType>>
    class TDynamicEnumMap
        : public TDynamicDirectMap<const typename EnumStructType::EType*, ValueType, AllocType>
    {
    public:
        using TEnumStruct = EnumStructType;
        using TEnum = typename TEnumStruct::EType;
        using TValue = ValueType;
        using TValueParam = typename TTypeTraits<TValue>::TFuncParam;
        using TAlloc = AllocType;

        using TBase = TDynamicDirectMap<const TEnum*, TValue, TAlloc>;

    private:
        TDynamicEnumMap(const TArrayRef<const TEnum>& domain,
            const NDetail::TDomainTraits<TEnum>& traits,
            TValueParam value,
            const TAlloc& alloc)
            : TBase(domain.begin(), domain.end(),
                traits.Offset, value,
                traits.Size, alloc)
        {}

    public:
        TDynamicEnumMap(const TArrayRef<const TEnum>& domain, TValueParam value,
            const TAlloc& alloc = TAlloc())
            : TDynamicEnumMap(domain, NDetail::TDomainTraits<TEnum>(domain), value, alloc)
        {}
        explicit TDynamicEnumMap(const TArrayRef<const TEnum>& domain,
            const TAlloc& alloc = TAlloc())
            : TDynamicEnumMap(domain, TValue{}, alloc)
        {}
        explicit TDynamicEnumMap(TValueParam value = TValue{},
            const TAlloc& alloc = TAlloc())
            : TDynamicEnumMap(TEnumStruct::GetValuesRegion(), value, alloc)
        {}
    };

    template <typename EnumStructType, typename ValueType, typename AllocType = std::allocator<ValueType>>
    using TEnumMap = TDynamicEnumMap<EnumStructType, ValueType, AllocType>;

    template <typename EnumStructType, typename ValueType, typename AllocType = TPoolAlloc<ValueType>>
    class TPoolableEnumMap
        : public TPoolableDirectMap<const typename EnumStructType::EType*, ValueType, AllocType>
    {
    public:
        using TEnumStruct = EnumStructType;
        using TEnum = typename TEnumStruct::EType;
        using TValue = ValueType;
        using TValueParam = typename TTypeTraits<TValue>::TFuncParam;
        using TAlloc = AllocType;

        using TBase = TPoolableDirectMap<const TEnum*, TValue, TAlloc>;

    private:
        template <typename PoolType>
        TPoolableEnumMap(PoolType& pool, const TArrayRef<const TEnum>& domain,
            const NDetail::TDomainTraits<TEnum>& traits,
            TValueParam value)
            : TBase(pool, domain.begin(), domain.end(),
                traits.Offset, value,
                traits.Size)
        {}

    public:
        TPoolableEnumMap() = default;

        template <typename PoolType>
        TPoolableEnumMap(PoolType& pool, const TArrayRef<const TEnum>& domain, TValueParam value)
            : TPoolableEnumMap(pool, domain, NDetail::TDomainTraits<TEnum>(domain), value)
        {}
        template <typename PoolType>
        TPoolableEnumMap(PoolType& pool, const TArrayRef<const TEnum>& domain)
            : TPoolableEnumMap(pool, domain, TValue{})
        {}
        template <typename PoolType>
        explicit TPoolableEnumMap(PoolType& pool, TValueParam value = TValue{})
            : TPoolableEnumMap(pool, TEnumStruct::GetValuesRegion(), value)
        {}
    };

    template <typename EnumStructType, typename ValueType>
    using TEnumMapView = TDirectMapView<const typename EnumStructType::EType*, ValueType>;

    //
    // Compact enum maps
    //

    template <typename EnumStructType, typename AllocType = std::allocator<i32>>
    class TDynamicCompactEnumRemap
        : public TDynamicCompactRemap<const typename EnumStructType::EType*, AllocType>
    {
    public:
        using TEnumStruct = EnumStructType;
        using TEnum = typename TEnumStruct::EType;
        using TAlloc = AllocType;

        using TBase = TDynamicCompactRemap<const TEnum*, TAlloc>;

    private:
        TDynamicCompactEnumRemap(const TArrayRef<const TEnum>& domain,
            const NDetail::TDomainTraits<TEnum>& traits,
            const TAlloc& alloc)
            : TBase(domain.begin(), domain.end(),
                traits.Offset, traits.Size, alloc)
        {}

    public:
        TDynamicCompactEnumRemap(const TArrayRef<const TEnum>& domain, const TAlloc& alloc = TAlloc())
            : TDynamicCompactEnumRemap(domain, NDetail::TDomainTraits<TEnum>(domain), alloc)
        {}
        explicit TDynamicCompactEnumRemap(const TAlloc& alloc = TAlloc())
            : TDynamicCompactEnumRemap(TEnumStruct::GetValuesRegion(), alloc)
        {}
    };

    template <typename EnumStructType, typename AllocType = std::allocator<i32>>
    using TCompactEnumRemap = TDynamicCompactEnumRemap<EnumStructType, AllocType>;

    template <typename EnumStructType, typename AllocType = TPoolAlloc<i32>>
    class TPoolableCompactEnumRemap
        : public TPoolableCompactRemap<const typename EnumStructType::EType*, AllocType>
    {
    public:
        using TEnumStruct = EnumStructType;
        using TEnum = typename TEnumStruct::EType;
        using TAlloc = AllocType;

        using TBase = TPoolableCompactRemap<const TEnum*, TAlloc>;

    private:
        template <typename PoolType>
        TPoolableCompactEnumRemap(PoolType& pool, const TArrayRef<const TEnum>& domain,
            const NDetail::TDomainTraits<TEnum>& traits)
            : TBase(pool, domain.begin(), domain.end(),
                traits.Offset, traits.Size)
        {}

    public:
        TPoolableCompactEnumRemap() = default;

        template <typename PoolType>
        TPoolableCompactEnumRemap(PoolType& pool, const TArrayRef<const TEnum>& domain)
            : TPoolableCompactEnumRemap(pool, domain, NDetail::TDomainTraits<TEnum>(domain))
        {}
        template <typename PoolType>
        explicit TPoolableCompactEnumRemap(PoolType& pool)
            : TPoolableCompactEnumRemap(pool, TEnumStruct::GetValuesRegion())
        {}
    };

    template <typename EnumStructType>
    using TCompactEnumRemapView = TCompactRemapView<const typename EnumStructType::EType*>;

    //
    // Compact enum maps
    //

    template <typename EnumStructType, typename ValueType, typename AllocType = std::allocator<ValueType>>
    class TDynamicCompactEnumMap
        : public TDynamicCompactMap<const typename EnumStructType::EType*, ValueType, AllocType>
    {
    public:
        using TEnumStruct = EnumStructType;
        using TEnum = typename TEnumStruct::EType;
        using TRemapView = TCompactEnumRemapView<TEnumStruct>;
        using TValue = ValueType;
        using TValueParam = typename TTypeTraits<TValue>::TFuncParam;
        using TAlloc = AllocType;

        using TBase = TDynamicCompactMap<const TEnum*, TValue, TAlloc>;

    public:
        TDynamicCompactEnumMap(const TRemapView& view,
            TValueParam value,
            const TAlloc& alloc = TAlloc())
            : TBase(view.KeysBegin(), view.KeysEnd(),
                view, value, view.Size(), alloc)
        {}
        explicit TDynamicCompactEnumMap(const TRemapView& view,
            const TAlloc& alloc = TAlloc())
            : TDynamicCompactEnumMap(view, TValue{}, alloc)
        {}
        explicit TDynamicCompactEnumMap(const TAlloc& alloc = TAlloc())
            : TDynamicCompactEnumMap(TEnumStruct::GetFullRemap().View(), TValue{}, alloc)
        {}
    };

    template <typename EnumStructType, typename ValueType, typename AllocType = std::allocator<ValueType>>
    using TCompactEnumMap = TDynamicCompactEnumMap<EnumStructType, ValueType, AllocType>;

    template <typename EnumStructType, typename ValueType, typename AllocType = TPoolAlloc<ValueType>>
    class TPoolableCompactEnumMap
        : public TPoolableCompactMap<const typename EnumStructType::EType*, ValueType, AllocType>
    {
    public:
        using TEnumStruct = EnumStructType;
        using TEnum = typename TEnumStruct::EType;
        using TRemapView = TCompactEnumRemapView<TEnumStruct>;
        using TValue = ValueType;
        using TValueParam = typename TTypeTraits<TValue>::TFuncParam;
        using TAlloc = AllocType;

        using TBase = TPoolableCompactMap<const TEnum*, TValue, TAlloc>;

    public:
        TPoolableCompactEnumMap() = default;

        template <typename PoolType>
        TPoolableCompactEnumMap(PoolType& pool, const TRemapView& view,
            TValueParam value = TValue{})
            : TBase(pool, view.KeysBegin(), view.KeysEnd(),
                view, value, view.Size())
        {}
        template <typename PoolType>
        explicit TPoolableCompactEnumMap(PoolType& pool,
            TValueParam value = TValue{})
            : TBase(pool, TEnumStruct::GetFullRemap().View(), value)
        {}
    };

    template <typename EnumStructType, typename ValueType>
    using TCompactEnumMapView =  TCompactMapView<const typename EnumStructType::EType*, ValueType>;
}
