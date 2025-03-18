#pragma once

#include <type_traits>


namespace NAntiRobot {


template <typename... Ts>
struct TParamPack;

template <>
struct TParamPack<> {
    template <template <typename... Us> typename X>
    using TApply = X<>;

    template <typename TOtherPack>
    using TConcat = TOtherPack;

    template <template <typename U> typename TPred>
    using TFilter = TParamPack<>;

    template <typename U>
    static constexpr bool Contains = false;

    template <template <typename> typename TPred>
    static constexpr bool ContainsIf = false;

    template <typename U>
    static constexpr int IndexOf = -1;

    template <template <typename> typename TPred>
    static constexpr int IndexIf = -1;
};

template <typename T, typename... Ts>
struct TParamPack<T, Ts...> {
public:
    using THead = T;
    using TTail = TParamPack<Ts...>;

private:
    template <typename... Us>
    struct TDoConcat {
        using TType = TParamPack<T, Ts..., Us...>;
    };

    template <bool C, typename U>
    struct TFilterOne {
        using TType = TParamPack<>;
    };

    template <typename U>
    struct TFilterOne<true, U> {
        using TType = TParamPack<U>;
    };

public:
    template <template <typename...> typename X>
    using TApply = X<T, Ts...>;

    template <typename TOtherPack>
    using TConcat = typename TOtherPack::template TApply<TDoConcat>::TType;

    template <template <typename> typename TPred>
    using TFilter = typename TFilterOne<TPred<T>::value, T>::TType
        ::template TConcat<
            typename TTail::template TFilter<TPred>
        >;

    template <typename U>
    static constexpr bool Contains = std::is_same_v<T, U> || TTail::template Contains<U>;

    template <template <typename> typename TPred>
    static constexpr bool ContainsIf = TPred<T>::value || TTail::template ContainsIf<TPred>;

    template <typename U>
    static constexpr int IndexOf = std::is_same_v<T, U> ? 0 : 1 + TTail::template IndexOf<U>;

    template <template <typename> typename TPred>
    static constexpr int IndexIf = TPred<T>::value ? 0 : 1 + TTail::template IndexIf<TPred>;
};


} // namespace NAntiRobot
