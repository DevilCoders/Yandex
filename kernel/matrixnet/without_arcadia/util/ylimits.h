#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#include <limits>

template <class T>
static T Max() {
    return std::numeric_limits<T>::max();
}

template <class T>
static T Min() {
    return std::numeric_limits<T>::min();
}
