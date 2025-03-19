#pragma once

#include <library/cpp/wordpos/wordpos.h>

namespace NDoom {


/**
 * Sizes of common hit fields.
 */
enum EFieldSize {
    CounterTypeSize = 2,
    StreamTypeSize = 6,
    TermIdSize = 30,
    DocIdSize = DOC_LEVEL_Bits,
    BreakSize = BREAK_LEVEL_Bits,
    WordSize = WORD_LEVEL_Bits,
    FormSize = 7,
    RegionSize = 15,
};

} // namespace NDoom
