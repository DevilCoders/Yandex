#pragma once

#define DEF_SHIFT_AND_MASK(name, offset)    \
    CM_ ## name ## _OFFSET = (offset),      \
    CM_ ## name = 1 << (offset),

namespace NClassificationRule {
    enum ClassMask {
        DEF_SHIFT_AND_MASK(DOWNLOAD,    0)
        DEF_SHIFT_AND_MASK(BRANDNAMES,  1)
        DEF_SHIFT_AND_MASK(DISEASE,     2)
        DEF_SHIFT_AND_MASK(KAK,         3)

        DEF_SHIFT_AND_MASK(MOSCOW,      4)
        DEF_SHIFT_AND_MASK(OAO,         5)
        DEF_SHIFT_AND_MASK(PORNO,       6)
        DEF_SHIFT_AND_MASK(TRAVEL,      7)

        DEF_SHIFT_AND_MASK(INFO,        8)

        DEF_SHIFT_AND_MASK(GAMES,     9)
        DEF_SHIFT_AND_MASK(EDUCATION, 10)
        DEF_SHIFT_AND_MASK(MUSIC,     11)
        DEF_SHIFT_AND_MASK(NEWS,      12)
        DEF_SHIFT_AND_MASK(WATCH,     13)

        DEF_SHIFT_AND_MASK(JOB,     14)
    };
}

#undef DEF_SHIFT_AND_MASK
