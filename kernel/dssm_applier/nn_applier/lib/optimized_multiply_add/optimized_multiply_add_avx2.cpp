#include <kernel/dssm_applier/nn_applier/lib/optimized_multiply_add/optimized_multiply_add.h>

#include <util/generic/xrange.h>
#include <util/system/unaligned_mem.h>

#include <xmmintrin.h>
#include <emmintrin.h>
#include <immintrin.h>

// prepared on godbold set https://godbolt.org/z/vjmxZN
// NOTE 1: wihtout compile-time known dimension clang fails to exclude storing in mem vectorizedRes
// NOTE 2: if dimension <= 48 is compile-time known and MaxUsedRegisters <= 6, then all storings are removed,
//    state is completely hold at registers https://godbolt.org/z/pF__-t
// so, some additional optimization for small dimensions can be performed
// if avx512 are available, optimization can be performed for bigger dimensions

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    )
{
    constexpr size_t MaxUsedRegisters = 32;
    float shiftCoeff = (min + 0.5f * coeff);
    float globalAdd = 0;

    constexpr size_t floatsInRegister = 256 / 32;
    const size_t upperBoundOfRegistersToUse = 1 + (dimension - 1) / floatsInRegister;
    Y_ASSERT(upperBoundOfRegistersToUse <= MaxUsedRegisters);
    const ui8* simplifiedLoadBorder = data.end() - floatsInRegister * upperBoundOfRegistersToUse;

    __m256 vectorizedRes[MaxUsedRegisters];
    for(auto i : xrange(upperBoundOfRegistersToUse)) {
        vectorizedRes[i] = _mm256_setzero_ps();
    }

    for(auto k : xrange(multiplyValues.size())) {
        __m256 mult = _mm256_set1_ps(multiplyValues[k] * coeff);
        globalAdd += multiplyValues[k] * shiftCoeff;

        const ui8* row = data.begin() + valuesToLookup[k] * dimension;
        Y_ASSERT(row < data.end());

        if (Y_LIKELY(row < simplifiedLoadBorder)) {
            for(auto i : xrange(upperBoundOfRegistersToUse)) {
                const __m256i bytesConvertedInteger = _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(ReadUnaligned<i64>(row)));
                const __m256 asFloats = _mm256_cvtepi32_ps(bytesConvertedInteger);
                vectorizedRes[i] = _mm256_add_ps(_mm256_mul_ps(mult, asFloats), vectorizedRes[i]);
                row += floatsInRegister;
            }
        } else {
            for(auto i : xrange(upperBoundOfRegistersToUse - 1)) {
                __m256i bytesConvertedInteger = _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(ReadUnaligned<i64>(row)));
                __m256 asFloats = _mm256_cvtepi32_ps(bytesConvertedInteger);
                vectorizedRes[i] = _mm256_add_ps(_mm256_mul_ps(mult, asFloats), vectorizedRes[i]);
                row += floatsInRegister;
            }
            ui8 localBuf[floatsInRegister] = {0,0,0,0,0,0,0,0};
            for(auto i : xrange(dimension - (upperBoundOfRegistersToUse - 1) * floatsInRegister)) {
                localBuf[i] = row[i];
            }
            const __m256i bytesConvertedInteger = _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(ReadUnaligned<i64>(localBuf)));
            const __m256 asFloats = _mm256_cvtepi32_ps(bytesConvertedInteger);
            vectorizedRes[upperBoundOfRegistersToUse - 1] = _mm256_add_ps(_mm256_mul_ps(mult, asFloats), vectorizedRes[upperBoundOfRegistersToUse - 1]);
        }
    }

    const __m256 vecGlobalAdd = _mm256_set1_ps(globalAdd);
    for(auto i : xrange(upperBoundOfRegistersToUse)) {
        vectorizedRes[i] = _mm256_add_ps(vectorizedRes[i], vecGlobalAdd);
    }
    if (upperBoundOfRegistersToUse * floatsInRegister == dimension) {
        for(auto i : xrange(upperBoundOfRegistersToUse)) {
            _mm256_storeu_ps(dst.begin() + floatsInRegister * i, vectorizedRes[i]);
        }
    } else {
        float* shiftedDst = dst.begin();
        for(auto i : xrange(upperBoundOfRegistersToUse - 1)) {
            _mm256_storeu_ps(shiftedDst, vectorizedRes[i]);
            shiftedDst += floatsInRegister;
        }
        float localBuf[floatsInRegister];
        _mm256_storeu_ps(localBuf, vectorizedRes[upperBoundOfRegistersToUse - 1]);
        for(auto i : xrange(dimension - (upperBoundOfRegistersToUse - 1) * floatsInRegister)) {
            shiftedDst[i] = localBuf[i];
        }
    }
}

template<size_t CompileTimeKnownDimensions>
static void DoMultiplyAddWithUnpackBatchedAVX2_CompileTimeDim(
        size_t dimensionInput,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    )
{
    constexpr size_t floatsInRegister = 256 / 32;
    constexpr size_t dimension = CompileTimeKnownDimensions;
    Y_ASSERT(dimensionInput == dimension);
    constexpr size_t MaxUsedRegisters = 1 + (dimension - 1) / floatsInRegister;
    const size_t upperBoundOfRegistersToUse = 1 + (dimension - 1) / floatsInRegister;

    float shiftCoeff = (min + 0.5f * coeff);
    float globalAdd = 0;

    Y_ASSERT(upperBoundOfRegistersToUse <= MaxUsedRegisters);
    const ui8* simplifiedLoadBorder = data.end() - floatsInRegister * upperBoundOfRegistersToUse;

    __m256 vectorizedRes[MaxUsedRegisters];
    for(auto i : xrange(upperBoundOfRegistersToUse)) {
        vectorizedRes[i] = _mm256_setzero_ps();
    }

    for(auto k : xrange(multiplyValues.size())) {
        __m256 mult = _mm256_set1_ps(multiplyValues[k] * coeff);
        globalAdd += multiplyValues[k] * shiftCoeff;

        const ui8* row = data.begin() + valuesToLookup[k] * dimension;
        Y_ASSERT(row < data.end());

        if (Y_LIKELY(row < simplifiedLoadBorder)) {
            for(auto i : xrange(upperBoundOfRegistersToUse)) {
                const __m256i bytesConvertedInteger = _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(ReadUnaligned<i64>(row)));
                const __m256 asFloats = _mm256_cvtepi32_ps(bytesConvertedInteger);
                vectorizedRes[i] = _mm256_add_ps(_mm256_mul_ps(mult, asFloats), vectorizedRes[i]);
                row += floatsInRegister;
            }
        } else {
            for(auto i : xrange(upperBoundOfRegistersToUse - 1)) {
                __m256i bytesConvertedInteger = _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(ReadUnaligned<i64>(row)));
                __m256 asFloats = _mm256_cvtepi32_ps(bytesConvertedInteger);
                vectorizedRes[i] = _mm256_add_ps(_mm256_mul_ps(mult, asFloats), vectorizedRes[i]);
                row += floatsInRegister;
            }
            ui8 localBuf[floatsInRegister] = {0,0,0,0,0,0,0,0};
            for(auto i : xrange(dimension - (upperBoundOfRegistersToUse - 1) * floatsInRegister)) {
                localBuf[i] = row[i];
            }
            const __m256i bytesConvertedInteger = _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(ReadUnaligned<i64>(localBuf)));
            const __m256 asFloats = _mm256_cvtepi32_ps(bytesConvertedInteger);
            vectorizedRes[upperBoundOfRegistersToUse - 1] = _mm256_add_ps(_mm256_mul_ps(mult, asFloats), vectorizedRes[upperBoundOfRegistersToUse - 1]);
        }
    }

    const __m256 vecGlobalAdd = _mm256_set1_ps(globalAdd);
    for(auto i : xrange(upperBoundOfRegistersToUse)) {
        vectorizedRes[i] = _mm256_add_ps(vectorizedRes[i], vecGlobalAdd);
    }
    if (upperBoundOfRegistersToUse * floatsInRegister == dimension) {
        for(auto i : xrange(upperBoundOfRegistersToUse)) {
            _mm256_storeu_ps(dst.begin() + floatsInRegister * i, vectorizedRes[i]);
        }
    } else {
        float* shiftedDst = dst.begin();
        for(auto i : xrange(upperBoundOfRegistersToUse - 1)) {
            _mm256_storeu_ps(shiftedDst, vectorizedRes[i]);
            shiftedDst += floatsInRegister;
        }
        float localBuf[floatsInRegister];
        _mm256_storeu_ps(localBuf, vectorizedRes[upperBoundOfRegistersToUse - 1]);
        for(auto i : xrange(dimension - (upperBoundOfRegistersToUse - 1) * floatsInRegister)) {
            shiftedDst[i] = localBuf[i];
        }
    }
}

//25% speed-up by benchmark
void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2_Dim40(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedAVX2_CompileTimeDim<40>(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2_Dim50(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedAVX2_CompileTimeDim<50>(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2_Dim60(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedAVX2_CompileTimeDim<60>(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2_Dim80(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedAVX2_CompileTimeDim<80>(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2_Dim100(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedAVX2_CompileTimeDim<100>(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2_Dim120(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedAVX2_CompileTimeDim<120>(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2_Dim150(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedAVX2_CompileTimeDim<150>(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2_Dim210(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedAVX2_CompileTimeDim<210>(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

