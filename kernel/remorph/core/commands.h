#pragma once

#include "types.h"

#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>
#include <util/stream/output.h>

namespace NRemorph {

struct TCommands {
    enum Type {
        Shift = 1,
        StorePosition = 2,
        StoreTag = 4
    };
    TTagId Tag;
    ui8 Flags;
    ui8 ShiftValue;
    ui8 StorePositionValue;
    ui8 StoreTagValue;

    struct TKey {
        Y_FORCE_INLINE TTagId operator ()(const TCommands& c) const {
            return c.Tag;
        }
    };

    TCommands()
        : Tag(0)
        , Flags(0)
        , ShiftValue(0)
        , StorePositionValue(0)
        , StoreTagValue(0)
    {
    }

    TCommands(TTagId tag)
        : Tag(tag)
        , Flags(0)
        , ShiftValue(0)
        , StorePositionValue(0)
        , StoreTagValue(0)
    {
    }

    void AddCommand(Type t, size_t v) {
        if (v > Max<ui8>()) {
            ythrow yexception() << "Too complex nfa";
        }
        switch (t) {
            case Shift:
                ShiftValue = (ui8)v;
                break;
            case StorePosition:
                StorePositionValue = (ui8)v;
                break;
            case StoreTag:
                StoreTagValue = (ui8)v;
                break;
        }
        Flags |= t;
    }
    bool HasCommand(Type t) const {
        return Flags & t;
    }
    ui8 GetValue(Type t) const {
        switch (t) {
            case Shift:
                return ShiftValue;
            case StorePosition:
                return StorePositionValue;
            case StoreTag:
                return StoreTagValue;
        }
        return 0;
    }
};

// Ensure we have no undesirable padding
static_assert(sizeof(TCommands) == 6, "expect sizeof(TCommands) == 6");

typedef NSorted::TSimpleSet<TCommands, TTagId, TCommands::TKey> TTagIdToCommands;

} // NRemorph

Y_DECLARE_PODTYPE(NRemorph::TCommands);

Y_DECLARE_OUT_SPEC(inline, NRemorph::TCommands, s, c) {
    s << (size_t)c.Tag << "->[" ;
    bool first = true;
    if (c.HasCommand(NRemorph::TCommands::Shift)) {
        if (first)
            first = false;
        s << "S" << (size_t)c.GetValue(NRemorph::TCommands::Shift);
    }
    if (c.HasCommand(NRemorph::TCommands::StorePosition)) {
        if (first)
            first = false;
        else
            s << ",";
        s << "P" << (size_t)c.GetValue(NRemorph::TCommands::StorePosition);
    }
    if (c.HasCommand(NRemorph::TCommands::StoreTag)) {
        if (first)
            first = false;
        else
            s << ",";
        s << "T" << (size_t)c.GetValue(NRemorph::TCommands::StoreTag);
    }
    s << "]";
}

Y_DECLARE_OUT_SPEC(inline, NRemorph::TTagIdToCommands, s, t) {
    s << "{";
    for (NRemorph::TTagIdToCommands::const_iterator i = t.begin(); i != t.end(); ++i) {
        if (i != t.begin())
            s << ",";
        s << *i;
    }
    s << "}";
}
