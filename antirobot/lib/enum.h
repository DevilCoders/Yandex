#pragma once

#include <type_traits>


namespace NAntiRobot {


template <typename T, typename U>
using TEnableIfEnum = std::enable_if_t<std::is_enum_v<T>, U>;


template <typename T>
using TEnumValueType = TEnableIfEnum<T, std::underlying_type_t<T>>;


template <typename T>
constexpr TEnumValueType<T> EnumValue(T x) {
    return static_cast<TEnumValueType<T>>(x);
}


} // namespace NAntiRobot
