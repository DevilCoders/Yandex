#include "tools.h"

#include <kernel/embeddings_info/proto/embedding.pb.h>

#include <kernel/dssm_applier/utils/utils.h>

#include <library/cpp/dot_product/dot_product.h>

#include <util/generic/ymath.h>
#include <util/ysaveload.h>
#include <util/stream/str.h>

namespace {
    void DoDecompress(const TString& data, const float multipler, TVector<float>& res) {
        const ui8* dataPtr = reinterpret_cast<const ui8*>(data.data());
        const TArrayRef<const ui8> compressed(dataPtr, data.size());
        res = NDssmApplier::NUtils::TFloat2UI8Compressor::Decompress(compressed);
        Transform(res.begin(), res.end(), res.begin(), [multipler](float f) {
            return f * multipler;
        });
    }

    void DoNormalizeAndCompress(const TVector<float>& data, NEmbedding::TEmbedding& emb) {
        const float norm = NNeuralNetApplier::GetL2Norm(data);
        if (!FuzzyEquals(1.f + norm, 1.f)) {
            float multipler;
            const TVector<ui8> compressed = NDssmApplier::NUtils::TFloat2UI8Compressor::Compress(data, &multipler);
            emb.SetEmbedding(reinterpret_cast<const char*>(compressed.data()), compressed.size());
            emb.SetMultipler(multipler / norm);
            emb.SetNorm(norm);
        } else {
            const TVector<char> zeroes(data.size(), 0);
            if (!zeroes.empty()) {
                emb.SetEmbedding(&zeroes.front(), zeroes.size());
            } else {
                emb.SetEmbedding("");
            }
            emb.SetMultipler(0.0);
            emb.SetNorm(0.0);
        }
    }

    void LoadEmbedding(const TString& data, TVector<float>& destination) {
        TStringInput in(data);
        destination.resize(data.size() / sizeof(float));
        LoadArray(&in, destination.data(), destination.size());
    }

    void SaveEmbedding(const TVector<float>& data, TString& destination) {
        destination.clear();
        TStringOutput out(destination);
        SaveArray(&out, data.data(), data.size());
    }
}

namespace NEmbeddingTools {
    using namespace NEmbedding;

    TEmbedding CreateEmptyEmbedding(const TEmbedding_EFormat compression) {
        TEmbedding emb;
        Init(emb, compression);
        return emb;
    }

    void Init(TEmbedding& embedding, const TEmbedding_EFormat compression) {
        embedding.SetFormat(compression);
    }

    TVector<float> GetVector(const TEmbedding& emb) {
        TVector<float> res;
        switch (emb.GetFormat()) {
            case TEmbedding_EFormat::TEmbedding_EFormat_Compressed:
                DoDecompress(emb.GetEmbedding(), emb.GetMultipler(), res);
                break;
            case TEmbedding_EFormat::TEmbedding_EFormat_Decompressed:
                LoadEmbedding(emb.GetEmbedding(), res);
                break;
            default:
                ythrow yexception() << "wrong embedding format";
        }
        return res;
    }

    void SetVector(const TVector<float>& data, TEmbedding& emb) {
        switch (emb.GetFormat()) {
            case TEmbedding_EFormat::TEmbedding_EFormat_Compressed:
                DoNormalizeAndCompress(data, emb);
                break;
            case TEmbedding_EFormat::TEmbedding_EFormat_Decompressed:
                SaveEmbedding(data, *emb.MutableEmbedding());
                emb.ClearMultipler();
                break;
            default:
                ythrow yexception() << "wrong embedding format";
        }
    }

    void Compress(TEmbedding& emb) {
        Y_ASSERT(emb.GetFormat() == TEmbedding_EFormat::TEmbedding_EFormat_Decompressed);
        if (emb.GetFormat() == TEmbedding_EFormat::TEmbedding_EFormat_Decompressed) {
            TVector<float> decompressed;
            LoadEmbedding(emb.GetEmbedding(), decompressed);
            DoNormalizeAndCompress(std::move(decompressed), emb);
            emb.SetFormat(TEmbedding_EFormat::TEmbedding_EFormat_Compressed);
        }
    }

    void Decompress(TEmbedding& emb) {
        Y_ASSERT(emb.GetFormat() == TEmbedding_EFormat::TEmbedding_EFormat_Compressed);
        if (emb.GetFormat() == TEmbedding_EFormat::TEmbedding_EFormat_Compressed) {
            TVector<float> decompressed;
            DoDecompress(emb.GetEmbedding(), emb.GetMultipler(), decompressed);
            SaveEmbedding(decompressed, *emb.MutableEmbedding());
            emb.ClearMultipler();
            emb.SetFormat(TEmbedding_EFormat::TEmbedding_EFormat_Decompressed);
        }
    }
}
