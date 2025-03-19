#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/fwd.h>

namespace NNeuralNetOps {
    TVector<ui8> Compress(const TVector<float>& embedding, const TVector<float>& weights, bool saveNorm = false);

    void Decompress(const TArrayRef<const ui8>& compressed, TVector<float>& embedding, TVector<float>& weights, TMaybe<float>& norm, size_t expectedEmbedSize);

    size_t GetCompressedBlockActualSize(const TArrayRef<const ui8>& compressed, size_t expectedEmbedSize);
} // NNeuralNetOps
