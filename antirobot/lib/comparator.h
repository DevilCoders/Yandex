#pragma once

#include <utility>


namespace NAntiRobot {


enum class EComparatorOp {
    Nop,
    Less,
    LessEqual,
    Equal,
    NotEqual,
    Greater,
    GreaterEqual
};


template <typename T>
struct TComparator {
    EComparatorOp Op = EComparatorOp::Nop;

    TComparator() = default;

    explicit TComparator(EComparatorOp op) : Op(op) {}

    bool Compare(const T& x, const T& y) const {
        switch (Op) {
        case EComparatorOp::Nop:
            return true;
        case EComparatorOp::Less:
            return x < y;
        case EComparatorOp::LessEqual:
            return x <= y;
        case EComparatorOp::Equal:
            return x == y;
        case EComparatorOp::NotEqual:
            return x != y;
        case EComparatorOp::Greater:
            return x > y;
        case EComparatorOp::GreaterEqual:
            return x >= y;
        }
    }
};


template <typename T>
struct TConstantComparator {
    EComparatorOp Op = EComparatorOp::Nop;
    T Constant{};

    TConstantComparator() = default;

    TConstantComparator(EComparatorOp op, T constant)
        : Op(op)
        , Constant(std::move(constant))
    {}

    auto operator<=>(const TConstantComparator&) const = default;

    bool Compare(const T& x) const {
        return TComparator<T>(Op).Compare(x, Constant);
    }

    bool IsNop() const {
        return Op == EComparatorOp::Nop;
    }
};


template <typename TValue>
struct TInexactFloatPrecision3 {
    static constexpr TValue Value = 1e-3;
};


template <typename TValue, typename TPrecision>
struct TInexactFloat {
    TValue Value{};

    TInexactFloat() = default;

    TInexactFloat(TValue value)  // NOLINT(google-explicit-constructor)
        : Value(value)
    {}

    auto operator<=>(TInexactFloat that) const {
        return Value <=> that.Value;
    }

    bool operator==(TInexactFloat that) const {
        return std::fabs(Value - that.Value) < TPrecision::Value;
    }

    bool operator!=(TInexactFloat that) const {
        return std::fabs(Value - that.Value) >= TPrecision::Value;
    }
};


using TInexactFloat3 = TInexactFloat<float, TInexactFloatPrecision3<float>>;


} // namespace NAntiRobot
