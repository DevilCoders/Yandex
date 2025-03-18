#pragma once

#include <library/cpp/containers/absl_flat_hash/flat_hash_set.h>

#include <util/generic/hash.h>


namespace NAntiRobot {


template <typename K, typename H = THash<K>, typename... TRest>
using TAbslFlatHashSet = absl::flat_hash_set<K, H, TRest...>;


} // namespace NAntiRobot
