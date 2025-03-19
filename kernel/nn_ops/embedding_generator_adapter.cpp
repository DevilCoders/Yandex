#include "embedding_generator_adapter.h"

#include <kernel/dssm_applier/decompression/dssm_model_decompression.h>

#include <util/generic/vector.h>
#include <library/cpp/string_utils/url/url.h>

namespace NNeuralNetOps {
    IEmbeddingGeneratorAdapter::~IEmbeddingGeneratorAdapter() {
    }

    TEmbeddingGeneratorAdapter::TEmbeddingGeneratorAdapter(const NDssmApplier::EDssmModelType modelType, const TString& filename)
        : EmbeddingGenerator(modelType)
    {
        Y_ENSURE(filename, "TEmbeddingGeneratorAdapter: Path to dssm model is required.");
        EmbeddingGenerator.Init(filename);
    }

    NDssmApplier::EDssmModelType TEmbeddingGeneratorAdapter::GetModelType() const noexcept {
        return EmbeddingGenerator.GetModelType();
    }

    NNeuralNetApplier::TVersionRange TEmbeddingGeneratorAdapter::GetSupportedVersions() const noexcept {
        return EmbeddingGenerator.GetSupportedVersions();
    }

    TVector<ui8> TEmbeddingGeneratorAdapter::GenerateEmbedding(const TString& query, const TString& url,
        const TString& title) const
    {
        switch (GetModelType()) {
            case NDssmApplier::EDssmModelType::LogDwellTimeBigrams:
                return EmbeddingGenerator.GenerateEmbedding(url, title);
            case NDssmApplier::EDssmModelType::FpsSpylogAggregatedQueryPart:
                {
                    TString host, path;
                    SplitUrlToHostAndPath(url, host, path);
                    static const TVector<TString> annotations = {
                        "AggregatedHosts", "AggregatedPaths", "AggregatedTitles"
                    };
                    static const TVector<TString> output = {
                        "query_embedding"
                    };
                    return EmbeddingGenerator.GenerateEmbedding(
                        annotations,
                        TVector<TString>{host, path, title},
                        output
                    );
                }
            case NDssmApplier::EDssmModelType::FpsSpylogAggregatedDocPart:
            case NDssmApplier::EDssmModelType::UserHistoryHostClusterDocPart:
                {
                    TString host, path;
                    SplitUrlToHostAndPath(url, host, path);
                    static const TVector<TString> annotations = {
                        "Host", "Path", "Title"
                    };
                    static const TVector<TString> output = {
                        "doc_embedding"
                    };
                    return EmbeddingGenerator.GenerateEmbedding(
                        annotations,
                        TVector<TString>{host, path, title},
                        output
                    );
                }
            case NDssmApplier::EDssmModelType::ReformulationsQueryEmbedderMini:
                {
                    static const TVector<TString> annotations = { "query" };
                    static const TVector<TString> output = { "query_embedding" };
                    return EmbeddingGenerator.GenerateEmbedding(
                        annotations,
                        TVector<TString>{ query },
                        output
                    );
                }
            default:
                {
                    static const TVector<TString> annotations = {
                        "query", "url", "title"
                    };
                    return EmbeddingGenerator.GenerateEmbedding(
                        annotations,
                        TVector<TString>{"", url, title}
                    );
                }
        }
    }
}
