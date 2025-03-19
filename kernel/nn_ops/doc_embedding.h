#pragma once

/// @file doc_embedding.h DSSM-embedding generation for documents. See https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/relevance/dssm/

#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <kernel/dssm_applier/decompression/decompression.h>

namespace NNeuralNetOps {

    using EModelType = NDssmApplier::EDssmModelType;

    /** Generates DSSM-embeddings using doc's url and title based on a model file.
     *
     *  The embedding is a vector, each item is remapped to [0-255] interval (converted to byte).
     * */
    class TEmbeddingGenerator {
    public:
        explicit TEmbeddingGenerator(EModelType modelType) noexcept
            : ModelType{modelType}
        {
        }

        void Init(const TString& filename);
        void Init(TBlob model);

        // Common generation method
        TVector<ui8> GenerateEmbedding(const TVector<TString>& annotations, const TVector<TString>& values, const TVector<TString>& output = DefaultOutputLayer) const;

        // EModelType::LogDwellTimeBigrams specific generation method
        // (implies url normalization)
        TVector<ui8> GenerateEmbedding(const TString& url, const TString& title) const;

        ui64 GetModelVersion() const noexcept;

        NDssmApplier::EDssmModelType GetModelType() const noexcept;

        NNeuralNetApplier::TVersionRange GetSupportedVersions() const noexcept;

    protected:
        static TString NormalizeUrl(const TString& url) noexcept;

    private:
        TVector<ui8> GenerateEmbedding(NNeuralNetApplier::TSample* samplePtr, const TVector<TString>& output) const;
        static NNeuralNetApplier::TSample* ConstructSample(const TString& url, const TString& title) noexcept;

    private:
        const EModelType ModelType;

        NNeuralNetApplier::TModel Model;
        static const TVector<TString> DefaultOutputLayer;
    };
} // NNeuralNetOps
