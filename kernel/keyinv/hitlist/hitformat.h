#pragma once

#include <util/system/defaults.h>
#include <util/system/compat.h>

using EHitFormat = ui16;

enum {
    YNDEX_VERSION_NWORD_FORM        = 2,    //!< Version that is currently used by robot.
    YNDEX_VERSION_RAW64_HITS        = 3,
    YNDEX_VERSION_BLK8              = 4,    //!< Version that is used by the search engine.
    YNDEX_VERSION_RT                = 5,
    YNDEX_VERSION_CURRENT           = YNDEX_VERSION_NWORD_FORM,

    // hit formats (max 16-bit value: 0xFFFF)
    HIT_FMT_UNKNOWN                 = -1,
    HIT_FMT_V2                      = YNDEX_VERSION_NWORD_FORM,
    HIT_FMT_RAW_I64                 = YNDEX_VERSION_RAW64_HITS,
    HIT_FMT_BLK8                    = YNDEX_VERSION_BLK8,
    HIT_FMT_RT                      = YNDEX_VERSION_RT,

    YNDEX_VERSION_MASK              = (1 << 16) - 1,

    YNDEX_VERSION_FLAG_KEY_COMPRESSION = 7 << 24,
    YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED_YAPPY = 1 << 24,
    YNDEX_VERSION_FLAG_KEY_2K = 2 << 24,


    YNDEX_VERSION_PORTION_DEFAULT = YNDEX_VERSION_NWORD_FORM,
    YNDEX_VERSION_FINAL_DEFAULT = (YNDEX_VERSION_BLK8 | YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED_YAPPY),
};

inline ui16 DetectHitFormat(ui32 indexVersion) {
    return static_cast<ui16>(indexVersion);
}
