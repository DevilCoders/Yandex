#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#include <string>

namespace NMatrixnet {

template<typename T>
std::string ToString(const T& value);

template<typename T>
T FromString(const std::string& s);

template<>
std::string ToString<double>(const double& value);

template<>
double FromString<double>(const std::string& s);

} // namespace NMatrixnet
