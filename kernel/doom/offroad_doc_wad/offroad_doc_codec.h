#pragma once

namespace NDoom {

enum EOffroadDocCodec {
    /** Standard offroad bit codec. Recommended default. */
    BitDocCodec,

    /** Adaptive codec uses bit encoding if there is <= 64 hits, and block codec otherwise. */
    AdaptiveDocCodec,
};

} // namespace NDoom
