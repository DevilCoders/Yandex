#pragma once

#include "parstypes.h"
#include "propface.h"
#include "event.h"
#include "zoneconf.h"

inline bool IsWordBreak(const THtmlChunk* ev) {
    BREAK_TYPE b = /**/ (BREAK_TYPE) /**/ ev->flags.brk;
    return b >= BREAK_WORD;
}

inline bool IsUsefulText(const THtmlChunk* ev) {
    TEXT_WEIGHT w = /**/ (TEXT_WEIGHT) /**/ ev->flags.weight;
    PARSED_TYPE t = /**/ (PARSED_TYPE) /**/ ev->flags.type;
    return t == PARSED_TEXT && w != WEIGHT_ZERO;
}

inline bool IsTitleText(const THtmlChunk* ev) {
    return IsUsefulText(ev) && WEIGHT_BEST == ev->flags.weight;
}

inline bool IsTerminationEvent(const THtmlChunk* ev) {
    return ev->flags.type < 0;
}

inline bool IsOriginalText(const THtmlChunk* ev) {
    const PARSED_TYPE t = /**/ (PARSED_TYPE) /**/ ev->flags.type;
    const MARKUP_TYPE m = /**/ (MARKUP_TYPE) /**/ ev->flags.markup;
    return m != MARKUP_IMPLIED &&
           (t == PARSED_TEXT || t == PARSED_MARKUP);
}
