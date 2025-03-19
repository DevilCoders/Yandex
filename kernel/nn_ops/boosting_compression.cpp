#include "doc_embedding.h"

#include <kernel/dssm_applier/decompression/decompression.h>
#include <kernel/dssm_applier/utils/utils.h>

#include <library/cpp/dot_product/dot_product.h>

#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>

namespace NNeuralNetOps {

    namespace {
        void Decompress(const TArrayRef<const ui8>& compressed, NDssmApplier::EDssmModelType type, TVector<float>& result) noexcept {
            ::NDssmApplier::NUtils::Decompress(compressed, NDssmApplier::GetDecompression(type), &result);
        }

        TVector<float> Decompress(const TArrayRef<const ui8>& compressed, NDssmApplier::EDssmModelType type) noexcept {
            TVector<float> result;
            Decompress(compressed, type, result);
            return result;
        }
    }

    // format - saveNorm * 2^7 + n_weights, weights, embedding, norm(?)
    TVector<ui8> Compress(const TVector<float>& embedding, const TVector<float>& weights, bool saveNorm) {
        Y_ENSURE(weights.size() < 128, "More than 127 weights are not allowed");
        TVector<ui8> res(Reserve(embedding.size() + weights.size() + 1));
        res.push_back(weights.size() + (saveNorm << 7));

        const TVector<ui8>& w = NDssmApplier::NUtils::Compress(weights, 0, 1);
        res.insert(res.end(), w.begin(), w.end());

        for (auto x : embedding) {
            Y_ENSURE(x >= -1 && x <= 1);
        }
        const TVector<ui8>& normalized = NDssmApplier::NUtils::Compress(embedding, -1, 1);
        res.insert(res.end(), normalized.begin(), normalized.end());
        if (saveNorm) {
            const TVector<float>& decompressed = Decompress(normalized, NDssmApplier::EDssmModelType::DssmBoostingEmbeddings);
            float norm = DotProduct(decompressed.data(), decompressed.data(), decompressed.size());
            norm = sqrt(norm);

            ui8* normPtr = reinterpret_cast<ui8*>(&norm);
            res.insert(res.end(), normPtr, normPtr + sizeof(float));
        }
        return res;
    }

    size_t GetCompressedBlockActualSize(const TArrayRef<const ui8>& compressed, size_t expectedEmbedSize) {
        const ui8 nW = compressed[0] % 128;
        const bool saveNorm = compressed[0] >= 128;

        return 1 + nW + expectedEmbedSize + (saveNorm ? sizeof(float) : 0);
    }

    void Decompress(const TArrayRef<const ui8>& compressed, TVector<float>& embedding, TVector<float>& weights, TMaybe<float>& norm, size_t expectedEmbedSize) {
        norm.Clear();

        const ui8 nW = compressed[0] % 128;
        const bool saveNorm = compressed[0] >= 128;

        Y_ENSURE((compressed.size() > nW) && (compressed.size() > nW + expectedEmbedSize));

        Decompress(compressed.Slice(1, nW), NDssmApplier::EDssmModelType::DssmBoostingWeights, weights);
        Decompress(compressed.Slice(1 + nW, expectedEmbedSize), NDssmApplier::EDssmModelType::DssmBoostingEmbeddings, embedding);
        if (saveNorm) {
            const size_t embedEndIdx = 1 + nW + expectedEmbedSize;
            TArrayRef<const ui8> serializedNorm = compressed.Slice(embedEndIdx, compressed.size() - embedEndIdx);

            if (Y_LIKELY(serializedNorm.size() == sizeof(float))) {
                norm = ReadUnaligned<float>(serializedNorm.data());
            } else {
                Y_ENSURE(false, "Invalid serializedNorm size");
            }
        }
    }
} // NNeuralNetOps
