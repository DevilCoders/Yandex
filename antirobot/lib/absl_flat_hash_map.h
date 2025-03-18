#pragma once

#include <library/cpp/containers/absl_flat_hash/flat_hash_map.h>

#include <util/generic/hash.h>


namespace NAntiRobot {


template <typename K, typename V, typename H = THash<K>, typename... TRest>
using TAbslFlatHashMap = absl::flat_hash_map<K, V, H, TRest...>;


} // namespace NAntiRobot
