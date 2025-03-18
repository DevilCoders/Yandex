#pragma once

#include "ar_utils.h"

#include <util/digest/multi.h>

#include <compare>
#include <tuple>


namespace NAntiRobot {


// Theses types allow you to easily implement comparison operators by providing a method named
// AsTuple that returns a tuple of references.


template <typename TDerived, typename TTrait>
class TTupleMixin {
protected:
    TTupleMixin() = default;

    constexpr auto AsTuple() const {
        // The template parameter TTrait makes this cast unambiguous in case a type inherits from
        // multiple TTuple- traits.
        return static_cast<const TDerived*>(this)->AsTuple();
    }
};


template <typename... Ts>
using TOrderingFor = std::common_comparison_category_t<
    decltype(std::declval<Ts>() <=> std::declval<Ts>())...
>;


template <size_t Index, typename... Ts>
TOrderingFor<Ts...> DoCompareTuples(
    const std::tuple<Ts...>& left,
    const std::tuple<Ts...>& right
) {
    if constexpr (Index < sizeof...(Ts)) {
        const TOrderingFor<Ts...> order = std::get<Index>(left) <=> std::get<Index>(right);

        if (order != 0) {
            return order;
        }

        return DoCompareTuples<Index + 1>(left, right);
    } else if constexpr (Index == sizeof...(Ts)) {
        return TOrderingFor<Ts...>::equivalent;
    } else {
        static_assert(TValueDependentFalse<Index>, "Out of bounds");
    }
}


// The spaceship operator doesn't work with tuples for some reason – might be that we have
// incomplete C++20 support.
template <typename... Ts>
TOrderingFor<Ts...> CompareTuples(
    const std::tuple<Ts...>& left,
    const std::tuple<Ts...>& right
) {
    return DoCompareTuples<0, Ts...>(left, right);
}


template <typename TDerived>
class TTupleEquatable : public TTupleMixin<TDerived, TTupleEquatable<TDerived>> {
private:
    using TSelf = TTupleEquatable<TDerived>;

protected:
    TTupleEquatable() = default;

public:
    constexpr bool operator==(const TSelf& that) const {
        return AsTuple() == that.AsTuple();
    }

    constexpr bool operator!=(const TSelf& that) const {
        return AsTuple() != that.AsTuple();
    }

private:
    using TTupleMixin<TDerived, TTupleEquatable<TDerived>>::AsTuple;
};


template <typename TDerived>
class TTupleComparable : public TTupleEquatable<TDerived> {
private:
    using TSelf = TTupleComparable<TDerived>;

protected:
    TTupleComparable() = default;

public:
    // The spaceship operator is optional since some tuple members may not provide it.
    constexpr auto operator<=>(const TSelf& that) const {
        return CompareTuples(AsTuple(), that.AsTuple());
    }

    constexpr bool operator<(const TSelf& that) const {
        return AsTuple() < that.AsTuple();
    }

    constexpr bool operator<=(const TSelf& that) const {
        return AsTuple() <= that.AsTuple();
    }

    constexpr bool operator>(const TSelf& that) const {
        return AsTuple() > that.AsTuple();
    }

    constexpr bool operator>=(const TSelf& that) const {
        return AsTuple() >= that.AsTuple();
    }

    constexpr bool operator==(const TSelf& that) const {
        return AsTuple() == that.AsTuple();
    }

private:
    using TTupleMixin<TDerived, TTupleEquatable<TDerived>>::AsTuple;
};


template <typename TDerived>
class TTupleHashable : public TTupleMixin<TDerived, TTupleHashable<TDerived>> {
protected:
    TTupleHashable() = default;

public:
    struct THash {
        size_t operator()(const TTupleHashable<TDerived>& hashable) const {
            return hashable.Hash();
        }
    };

public:
    size_t Hash() const {
        return ::THash<decltype(AsTuple())>()(AsTuple());
    }

private:
    using TTupleMixin<TDerived, TTupleHashable<TDerived>>::AsTuple;
};


} // namespace NAntiRobot


template <typename T>
struct THash<NAntiRobot::TTupleHashable<T>> {
    size_t operator()(const NAntiRobot::TTupleHashable<T>& hashable) const {
        return hashable.Hash();
    }
};
