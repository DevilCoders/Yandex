#pragma once

#include <util/system/yassert.h>
#include <util/system/defaults.h>

#include <cstddef> //for size_t

namespace NSnippets {

    class TFactorBuf
    {
    private:
        float* Factors;
        size_t Count;
    public:
        TFactorBuf(float* factors, size_t count)
          : Factors(factors)
          , Count(count)
        {
        }
        float* Ptr() const {
            return Factors;
        }
        size_t Len() const {
            return Count;
        }
        float& operator[](size_t i) {
            Y_ASSERT(i < Len());
            return Ptr()[i];
        }
        const float& operator[](size_t i) const {
            Y_ASSERT(i < Len());
            return Ptr()[i];
        }
    };
}
