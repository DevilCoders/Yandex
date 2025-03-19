#pragma once
#include <util/generic/array_ref.h>
#include <util/system/cpu_id.h>

namespace NNeuralNetApplier {
    using FMultiplyAdd = void (*) (
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSimple(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSSE41(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSSE41_Dim40(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSSE41_Dim50(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSSE41_Dim60(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSSE41_Dim80(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSSE41_Dim100(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSSE41_Dim120(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSSE41_Dim150(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedSSE41_Dim210(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedAVX2(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedAVX2_Dim40(
        size_t dimensionInput,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedAVX2_Dim50(
        size_t dimensionInput,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedAVX2_Dim60(
        size_t dimensionInput,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedAVX2_Dim80(
        size_t dimensionInput,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedAVX2_Dim100(
        size_t dimensionInput,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedAVX2_Dim120(
        size_t dimensionInput,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );
    void DoMultiplyAddWithUnpackBatchedAVX2_Dim150(
        size_t dimensionInput,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );

    //only for tests
    void DoMultiplyAddWithUnpackBatchedAVX2_Dim210(
        size_t dimensionInput,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    );

    inline FMultiplyAdd GetImplForDoMultiplyAddWithUnpackBatched(size_t dimension) {
        if (NX86::CachedHaveAVX2() && dimension <= 256) {
            switch (ui8(dimension)) {
                case 40: return DoMultiplyAddWithUnpackBatchedAVX2_Dim40;
                case 50: return DoMultiplyAddWithUnpackBatchedAVX2_Dim50;
                case 60: return DoMultiplyAddWithUnpackBatchedAVX2_Dim60;
                case 80: return DoMultiplyAddWithUnpackBatchedAVX2_Dim80;
                case 100: return DoMultiplyAddWithUnpackBatchedAVX2_Dim100;
                case 120: return DoMultiplyAddWithUnpackBatchedAVX2_Dim120;
                case 150: return DoMultiplyAddWithUnpackBatchedAVX2_Dim150;
                case 210: return DoMultiplyAddWithUnpackBatchedAVX2_Dim210;
                default:
                    return DoMultiplyAddWithUnpackBatchedAVX2;
            }
        }
        if (NX86::CachedHaveSSE41() && dimension <= 256) {
            switch (ui8(dimension)) {
                case 40: return DoMultiplyAddWithUnpackBatchedSSE41_Dim40;
                case 50: return DoMultiplyAddWithUnpackBatchedSSE41_Dim50;
                case 60: return DoMultiplyAddWithUnpackBatchedSSE41_Dim60;
                case 80: return DoMultiplyAddWithUnpackBatchedSSE41_Dim80;
                case 100: return DoMultiplyAddWithUnpackBatchedSSE41_Dim100;
                case 120: return DoMultiplyAddWithUnpackBatchedSSE41_Dim120;
                case 150: return DoMultiplyAddWithUnpackBatchedSSE41_Dim150;
                case 210: return DoMultiplyAddWithUnpackBatchedSSE41_Dim210;
                default:
                    return DoMultiplyAddWithUnpackBatchedSSE41;
            }
        }
        return DoMultiplyAddWithUnpackBatchedSimple;
    }

    inline void DoMultiplyAddWithUnpackBatched(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    ) {
        GetImplForDoMultiplyAddWithUnpackBatched(dimension)(
            dimension, data, dst, multiplyValues, valuesToLookup, coeff, min
        );
    }
}
