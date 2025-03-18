#pragma once

#include "vec4.h"

#include <library/cpp/sse/sse.h>

/// A fast inverse square root. |Relative Error| <= 1.5 * 2^-12
inline float FastInvSqrt(float x) {
    return _mm_cvtss_f32(_mm_rsqrt_ps(_mm_set_ps1(x)));
}
