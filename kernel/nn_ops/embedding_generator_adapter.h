#pragma once
#include "doc_embedding.h"

#include <util/generic/fwd.h>

namespace NNeuralNetOps {

    /*
     * Interface to embedding generator adapter. Introduce to allow transparent result caching
     */
    class IEmbeddingGeneratorAdapter {
    public:
        virtual NDssmApplier::EDssmModelType GetModelType() const noexcept = 0;

        virtual NNeuralNetApplier::TVersionRange GetSupportedVersions() const noexcept = 0;


        virtual TVector<ui8> GenerateEmbedding(const TString& query, const TString& url, const TString& title) const = 0;

        virtual ~IEmbeddingGeneratorAdapter();
    };

    /*
     * Adapter to TEmbeddingGenerator, to hide ugly truth
     *
     * This class designed to be used for generating document embeddings
     */
    class TEmbeddingGeneratorAdapter: public IEmbeddingGeneratorAdapter {
    public:
        TEmbeddingGeneratorAdapter(NDssmApplier::EDssmModelType modelType, const TString& filename);

        NDssmApplier::EDssmModelType GetModelType() const noexcept override ;

        NNeuralNetApplier::TVersionRange GetSupportedVersions() const noexcept override ;

        /*
         * Recieve document related data and generate embedding. It is ok to add 1-2 more **document**
         * related fields to accommodate new models.
         *
         * All unsightly logic hidden inside
         */
        TVector<ui8> GenerateEmbedding(const TString& query, const TString& url, const TString& title) const override;

    private:
        TEmbeddingGenerator EmbeddingGenerator;
    };
}
