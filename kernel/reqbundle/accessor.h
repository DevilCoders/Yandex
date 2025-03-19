#pragma once

#include <util/generic/typetraits.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

namespace NReqBundle {
namespace NDetail {

    template <typename DataType>
    class TLightAccessor {
    public:
        using TData = DataType;
        using TNonConstData = std::remove_const_t<DataType>;
        using TConstData = const TNonConstData;

    public:
        TLightAccessor() = default;

        TLightAccessor(TData& data)
            : Data(&data)
        {}
        TLightAccessor(const TLightAccessor<TNonConstData>& accessor)
            : Data(accessor.Data)
        {}
        TLightAccessor(const TLightAccessor<TConstData>& accessor)
            : Data(accessor.Data)
        {}

        TLightAccessor& operator=(const TLightAccessor& accessor) = default;

        bool IsValid() const {
            return !!Data;
        }
        ui64 Id() const {
            return reinterpret_cast<ui64>(Data);
        }

    protected:
        TData& Contents() const {
            Y_ASSERT(Data);
            return *Data;
        }

        // this method is guaranteed to fail (at compile time) for const accessor
        TNonConstData& MutableContents() const {
            Y_ASSERT(Data);
            return *Data;
        }

    private:
        friend class TLightAccessor<TConstData>;
        friend class TLightAccessor<TNonConstData>;

        template <typename DataTypeY>
        friend DataTypeY& BackdoorAccess(TLightAccessor<DataTypeY> acc);

        TData* Data = nullptr;
    };

    // Name of this function is deliberately chosen
    // to be noticeable in source code.
    // Direct access to underlying data structure should
    // be required only in some special cases (e.g. serialization).
    //
    template <typename DataType>
    inline DataType& BackdoorAccess(TLightAccessor<DataType> acc) {
        return acc.Contents();
    }

    struct TIdentityCast {
        template <typename T>
        T& operator()(T& x) const {
            return x;
        }
    };

    struct TDereferenceCast {
        template <typename T>
        auto operator()(const T& x) const -> decltype(*x) {
            return *x;
        }
    };

    template <typename AccType, typename BaseIterType, typename CastType>
    class TAccessorIterator {
    public:
        using TBaseIterator = BaseIterType;
        using TAccessor = AccType;
        using TCast = CastType;
        using TSelf = TAccessorIterator<TAccessor, TBaseIterator, TCast>;

    public:
        TAccessorIterator(TBaseIterator iter, const TCast& cast)
            : Iter(iter)
            , Cast(cast)
        {}

        bool operator==(const TSelf& other) const {
            return Iter == other.Iter;
        }
        bool operator!=(const TSelf& other) const {
            return !(*this == other);
        }

        TAccessor operator*() const {
            return TAccessor(Cast(*Iter));
        }
        TSelf& operator++() {
            ++Iter;
            return *this;
        }

    private:
        TBaseIterator Iter;
        TCast Cast;
    };

    template <typename AccType, typename BaseContType, typename CastType>
    class TAccessorContainer {
    public:
        using TBaseContainer = BaseContType;
        using TAccessor = AccType;
        using TCast = CastType;
        using TBaseIterator = decltype(std::declval<TBaseContainer&>().begin());
        using TIterator = TAccessorIterator<TAccessor, TBaseIterator, TCast>;
        using TSelf = TAccessorContainer<TAccessor, TBaseContainer, TCast>;

    public:
        TAccessorContainer(TBaseContainer& cont, const TCast& cast = TCast())
            : Cont(&cont)
            , Cast(cast)
        {}

        TIterator begin() const {
            return TIterator(Cont->begin(), Cast);
        }
        TIterator end() const {
            return TIterator(Cont->end(), Cast);
        }

        size_t size() const {
            return Cont->size();
        }
        size_t Size() const {
            return Cont->Size();
        }

    private:
        TBaseContainer* Cont = nullptr;
        TCast Cast;
    };

    template <typename AccType, typename BaseContType, typename CastType>
    inline TAccessorContainer<AccType, BaseContType, CastType> MakeAccContainerWithCast(BaseContType& cont,
        const CastType& cast = CastType())
    {
        return TAccessorContainer<AccType, BaseContType, CastType>(cont, cast);
    }

    template <typename AccType, typename BaseContType>
    inline auto MakeAccContainer(BaseContType& cont) -> decltype(MakeAccContainerWithCast<AccType>(cont, TIdentityCast()))
    {
        return MakeAccContainerWithCast<AccType>(cont, TIdentityCast());
    }

    template <typename AccType, typename BaseContType>
    inline auto MakeAccPtrContainer(BaseContType& cont) -> decltype(MakeAccContainerWithCast<AccType>(cont, TDereferenceCast()))
    {
        return MakeAccContainerWithCast<AccType>(cont, TDereferenceCast());
    }
} // NDetail
} // NReqBundle
