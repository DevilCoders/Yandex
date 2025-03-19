#include <kernel/dssm_applier/nn_applier/lib/optimized_multiply_add/optimized_multiply_add.h>

//NOTE: this file is compiled on non x86 targets

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSSE41(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
    )
{
    DoMultiplyAddWithUnpackBatchedSimple(
        dimension,
        data,
        dst,
        multiplyValues,
        valuesToLookup,
        coeff,
        min
    );
}


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
    DoMultiplyAddWithUnpackBatchedSimple(
        dimension,
        data,
        dst,
        multiplyValues,
        valuesToLookup,
        coeff,
        min
    );
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedAVX2_Dim40(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
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
    return DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
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
    return DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
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
    return DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
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
    return DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
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
    return DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
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
    return DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
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
    return DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSSE41_Dim40(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSSE41_Dim50(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    return DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSSE41_Dim60(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSSE41_Dim80(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSSE41_Dim100(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSSE41_Dim120(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSSE41_Dim150(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}

void NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSSE41_Dim210(
        size_t dimension,
        TConstArrayRef<ui8> data,
        TArrayRef<float> dst,
        TConstArrayRef<float> multiplyValues,
        TConstArrayRef<size_t> valuesToLookup,
        float coeff,
        float min
) {
    DoMultiplyAddWithUnpackBatchedSimple(dimension, data, dst, multiplyValues, valuesToLookup, coeff, min);
}
