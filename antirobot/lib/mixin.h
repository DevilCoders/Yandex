#pragma once


namespace NAntiRobot {


// This is a trick to help implement common behavior without virtual methods.
template <typename TDerived, typename TTrait>
class TMixin {
protected:
    constexpr const TDerived& Derived() const {
        const auto trait = static_const<const TTrait*>(this);
        return static_cast<const TDerived&>(*trait);
    }

    constexpr TDerived& Derived() {
        const auto trait = static_cast<TTrait*>(this);
        return static_cast<TDerived&>(*this);
    }
};


} // namespace NAntiRobot
