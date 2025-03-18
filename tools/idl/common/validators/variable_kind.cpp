#include "variable_kind.h"

namespace yandex::maps::idl {

VariableKind methodVariableKind(bool isNativeMethod, bool isParameter)
{
    if (isNativeMethod) {
        return isParameter ?
            VariableKind::NativeMethodParameter :
            VariableKind::NativeMethodReturnValue;
    } else {
        return isParameter ?
            VariableKind::PlatformMethodParameter :
            VariableKind::PlatformMethodReturnValue;
    }
}

} // namespace yandex::maps::idl
