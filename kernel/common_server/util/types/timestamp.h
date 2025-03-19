#pragma once

#include <util/datetime/base.h>

template <class TDerived, class T = ui32>
class TCommonSeconds {
private:
    using TSelf = TCommonSeconds<TDerived, T>;

public:
    TCommonSeconds(TDerived value = TDerived::Zero()) noexcept
        : Value(value.Seconds())
    {
    }

    inline TDerived Get() const noexcept {
        return TDerived::Seconds(Value);
    }
    inline T Seconds() const noexcept {
        return Value;
    }
    inline double SecondsFloat() const noexcept {
        return Value;
    }

    inline TSelf& Set(TDerived value) noexcept {
        Value = value.Seconds();
        return *this;
    }
    inline TSelf& Set(TSelf other) noexcept {
        Value = other.Value;
        return *this;
    }

    inline explicit operator bool() const noexcept {
        return Value;
    }
    inline operator TDerived() const noexcept {
        return Get();
    }

    template <class TOther>
    inline TSelf& operator=(TOther&& other) noexcept {
        Set(std::forward<TOther>(other));
        return *this;
    }

    template <class TOther>
    inline bool operator<(TOther&& other) const noexcept {
        return Value < TSelf(other).Value;
    }
    template <class TOther>
    inline bool operator<=(TOther&& other) const noexcept {
        return Value <= TSelf(other).Value;
    }
    template <class TOther>
    inline bool operator>(TOther&& other) const noexcept {
        return Value > TSelf(other).Value;
    }
    template <class TOther>
    inline bool operator>=(TOther&& other) const noexcept {
        return Value >= TSelf(other).Value;
    }
    template <class TOther>
    inline bool operator!=(TOther&& other) const noexcept {
        return Value != TSelf(other).Value;
    }
    template <class TOther>
    inline bool operator==(TOther&& other) const noexcept {
        return Value == TSelf(other).Value;
    }

private:
    T Value;
};

using TSeconds = TCommonSeconds<TDuration>;
using TTimestamp = TCommonSeconds<TInstant>;

static_assert(sizeof(TSeconds) == 4);
static_assert(sizeof(TTimestamp) == 4);
