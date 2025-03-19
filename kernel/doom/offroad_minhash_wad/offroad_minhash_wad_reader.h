#pragma once

#include <kernel/doom/wad/wad.h>
#include <kernel/doom/wad/wad_index_type.h>

#include <library/cpp/offroad/flat/flat_searcher.h>

namespace NDoom {

template <typename T, typename Key, typename Data>
concept MinHashKeyValueReader = requires (const T& t, Key key, Data data, bool& res, const IWad* wad) {
    t.Reset(wad);
    res = t.Read(&key, &data);
};

} // namespace NDoom
