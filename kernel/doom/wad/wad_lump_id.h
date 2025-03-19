#pragma once

#include <tuple>

#include <util/digest/multi.h>

#include "wad_index_type.h"
#include "wad_lump_role.h"

namespace NDoom {

struct TWadLumpId {
    EWadIndexType Index;
    EWadLumpRole Role;

    TWadLumpId() = default;
    constexpr TWadLumpId(EWadIndexType index, EWadLumpRole role) : Index(index), Role(role) {}

    friend bool operator<(const TWadLumpId& l, const TWadLumpId& r) {
        return std::forward_as_tuple(l.Index, l.Role) < std::forward_as_tuple(r.Index, r.Role);
    }

    friend bool operator==(const TWadLumpId& l, const TWadLumpId& r) {
        return std::forward_as_tuple(l.Index, l.Role) == std::forward_as_tuple(r.Index, r.Role);
    }

    friend bool operator!=(const TWadLumpId& l, const TWadLumpId& r) {
        return !(l == r);
    }
};

} // namespace NDoom

template<>
struct THash<NDoom::TWadLumpId> {
    size_t operator()(const NDoom::TWadLumpId& id) const {
        return MultiHash(id.Index, id.Role);
    }
};
