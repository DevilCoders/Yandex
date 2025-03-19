#include "doc_embedding.h"

#include <kernel/nn_ops/doc_embedding.h_serialized.h>

#include <util/generic/cast.h>

#include <cmath>

namespace NNeuralNetOps {
    const TVector<TString> TEmbeddingGenerator::DefaultOutputLayer = {"doc_embedding"};

    void TEmbeddingGenerator::Init(TBlob blob) {
        Model.Load(blob);
        Model.Init();
    }

    void TEmbeddingGenerator::Init(const TString& fileName) {
        Init(TBlob::FromFile(fileName));
    }

    TVector<ui8> TEmbeddingGenerator::GenerateEmbedding(const TVector<TString>& annotations, const TVector<TString>& values, const TVector<TString>& output) const {
        Y_ENSURE(
            ModelType != EModelType::LogDwellTimeBigrams,
            "This GenerateEmbedding method is not for EModelType::LogDwellTimeBigrams model"
        );

        auto samplePtr = new NNeuralNetApplier::TSample(annotations, values);
        return GenerateEmbedding(samplePtr, output);
    }

    TVector<ui8> TEmbeddingGenerator::GenerateEmbedding(const TString& url, const TString& title) const {
        Y_ENSURE(
            ModelType == EModelType::LogDwellTimeBigrams,
            "This GenerateEmbedding method for EModelType::LogDwellTimeBigrams model only"
        );

        auto samplePtr = ConstructSample(url, title);
        return GenerateEmbedding(samplePtr, DefaultOutputLayer);
    }

    TVector<ui8> TEmbeddingGenerator::GenerateEmbedding(NNeuralNetApplier::TSample* samplePtr, const TVector<TString>& output) const {
        TVector<float> modelResult;
        // Be careful, Model takes ownership of raw samplePtr
        Model.Apply(samplePtr, output, modelResult);
        return ::NDssmApplier::Compress(modelResult, ModelType);

    }

    NDssmApplier::EDssmModelType TEmbeddingGenerator::GetModelType() const noexcept {
        return ModelType;
    }

    ui64 TEmbeddingGenerator::GetModelVersion() const noexcept {
        return Model.GetVersion();
    }

    NNeuralNetApplier::TVersionRange TEmbeddingGenerator::GetSupportedVersions() const noexcept {
        return Model.GetSupportedVersions();
    }

    TString TEmbeddingGenerator::NormalizeUrl(const TString& url) noexcept {
        size_t i = 0;
        while (i < url.size() && url[i] != '?' && url[i] != '%' && url[i] != '#' &&
            url[i] != '&' && url[i] != '=')
        {
            ++i;
        }
        return url.substr(0, i);
    }


    NNeuralNetApplier::TSample* TEmbeddingGenerator::ConstructSample(const TString& url, const TString& title) noexcept {
        static const TVector<TString> annotations = {"url", "title", "query"};
        const auto& urlNorm = NormalizeUrl(url);
        const TVector<TString> values = {urlNorm, title, ""};
        return new NNeuralNetApplier::TSample(annotations, values);
    }

} // NNeuralNetOps
