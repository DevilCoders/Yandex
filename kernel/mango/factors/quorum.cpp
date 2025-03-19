#include "quorum.h"

#include <util/generic/ymath.h>

namespace NMango
{

TWebDocQuorum::TWebDocQuorum(float mul)
{
    Weights[0] = WEIGHT0;
    Weights[1] = WEIGHT0;
    for (size_t i = 2; i < 100; ++i)
        Weights[i] = Min(1.f - (float)pow(0.01f, mul / sqrtf(static_cast<float>(i - 1))), WEIGHT0);
}

} // NMango
