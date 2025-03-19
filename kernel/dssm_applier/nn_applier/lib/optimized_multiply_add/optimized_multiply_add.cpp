#include "optimized_multiply_add.h"

#include <util/generic/xrange.h>

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSimple(
    size_t dimension,
    TConstArrayRef<ui8> data,
    TArrayRef<float> dst,
    TConstArrayRef<float> multiplyValues,
    TConstArrayRef<size_t> valuesToLookup,
    float coeff,
    float min
) {
    float shiftCoeff = (min + 0.5f * coeff);
    for(auto i : xrange(dimension)) {
        dst[i] = 0;
    }
    float globalAdd = 0;
    for(auto k : xrange(multiplyValues.size())) {
        float mult = multiplyValues[k] * coeff;
        globalAdd += multiplyValues[k] * shiftCoeff;
        const ui8* row = data.begin() + valuesToLookup[k] * dimension;
        Y_ASSERT(row < data.end());
        for(auto i : xrange(dimension)) {
            //NOTE: it is fact, that avx/sse-fma implementation of _fmadd_ps run exactly in this mode (with calcing in double, and storing in floats)
            //dst[i] = double(row[i]) * mult + dst[i];
            //but converting is two times slower
            //we decided for simplicity do not use _fmadd_ps insturctions
            dst[i] = row[i] * mult + dst[i];
        }
    }
    for(auto i : xrange(dimension)) {
        dst[i] += globalAdd;
    }
}
