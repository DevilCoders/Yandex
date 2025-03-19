#pragma once

#include <kernel/dssm_applier/begemot/production_data.h>
#include <kernel/dssm_applier/decompression/decompression.h>

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/vector.h>

namespace NDssmApplier {

    class TDssmEmbedding {
    private:
        TVector<float> Embedding_;

    public:
        TDssmEmbedding() = default;

        explicit TDssmEmbedding(TArrayRef<const float> embedding)
            : Embedding_(embedding.begin(), embedding.end())
        {
        }

        explicit TDssmEmbedding(TVector<float> embedding)
            : Embedding_(std::move(embedding))
        {
        }

        TDssmEmbedding(const TArrayRef<const char>& dataRegion, const TDecompression& decompression) {
            TArrayRef<const ui8> compressed(reinterpret_cast<const ui8*>(dataRegion.data()), dataRegion.size());
            Embedding_ = NUtils::Decompress(compressed, decompression);
        }

        float operator[](size_t idx) const {
            return Embedding_[idx];
        }

        float& operator[](size_t idx) {
            return Embedding_[idx];
        }

        size_t Size() const {
            return Embedding_.size();
        }

        float Norm() const {
            return 1.0f;
        }

        float Dot(const TDssmEmbedding& other) const {
            Y_ASSERT(Size() == other.Size());
            return DotProduct(Embedding_.begin(), other.Embedding_.begin(), Size());
        }

        const TVector<float>& GetRawEmbedding() const {
            return Embedding_;
        }

        // If embedding changed via operator[] and needs to be normalized.
        bool TryNormalize() {
            return NNeuralNetApplier::TryNormalize(Embedding_);
        }

        explicit operator bool() const {
            return !Embedding_.empty();
        }

        TString ToBase64() const {
            TString resultString;
            TStringOutput output(resultString);
            ::Save(&output, Embedding_);
            return Base64Encode(resultString);
        }

        template<NNeuralNetApplier::EDssmModel model>
        static TDssmEmbedding FromEncodedQueryEmedding(const TString& encoded, ui32 preferredVersion = 0) {
            TVector<float> rawEmbedding;
            THolder<NNeuralNetApplier::IBegemotDssmData> serializer = NNeuralNetApplier::GetDssmDataSerializer(model);
            const TString& decoded = Base64Decode(encoded);
            TMemoryInput input(decoded.data(), decoded.size());
            serializer->Load(&input);
            serializer->TryGetEmbedding(preferredVersion, rawEmbedding);
            if (!NNeuralNetApplier::TryNormalize(rawEmbedding)) {
                rawEmbedding.clear();
            }
            return TDssmEmbedding(std::move(rawEmbedding));
        }

        static TDssmEmbedding FromBase64(const TStringBuf& encoded) {
            TVector<float> rawEmbedding;
            TString decoded = Base64Decode(encoded);
            TMemoryInput input(decoded);
            ::Load(&input, rawEmbedding);
            return TDssmEmbedding(std::move(rawEmbedding));
        }

        static TDssmEmbedding FromRawEmbedding(TVector<float> rawEmbedding) {
            return TDssmEmbedding(std::move(rawEmbedding));
        }

        static TDssmEmbedding FromRawEmbedding(const float* ptr, size_t size) {
            return TDssmEmbedding(TVector<float>(ptr, ptr + size));
        }
    };

} // namespace NDssmApplier

