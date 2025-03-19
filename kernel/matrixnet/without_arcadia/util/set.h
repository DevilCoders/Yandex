#pragma once

#if !defined(MATRIXNET_WITHOUT_ARCADIA)
#error This header file is intended for use in the MATRIXNET_WITHOUT_ARCADIA mode only.
#endif

#include <set>

namespace NMatrixnet {

template<typename K>
using TSet = std::set<K>;

} // namespace NMatrixnet
