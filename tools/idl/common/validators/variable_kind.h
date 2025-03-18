#pragma once

namespace yandex::maps::idl {

/**
 * Represents different kinds of variables.
 *
 * Native methods are those of interfaces (implemented by MapKit), and
 * platform methods are those of listeners (implemented in a platform code).
 */
enum class VariableKind {
    Field, OptionsField,
    NativeMethodParameter, NativeMethodReturnValue,
    PlatformMethodParameter, PlatformMethodReturnValue
};

/**
 * Creates method-related variable (parameter or return value) kind.
 */
VariableKind methodVariableKind(bool isNativeMethod, bool isParameter);

} // namespace yandex::maps::idl
