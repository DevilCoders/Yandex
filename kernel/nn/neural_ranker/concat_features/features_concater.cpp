#include "features_concater.h"


namespace {
    namespace NProto {
        using namespace NNeuralRankerProtocol;
    }

    using TBatch = NNeuralRanker::TFeaturesConcater::TBatch;

    void AppendBatchSlice(
        const NNeuralRanker::NProto::TSlice& slice,
        const TBatch& batchFeatures,
        TVector<TVector<float>>& concatedBatch)
    {
        const auto& indices = slice.GetIndices();
        size_t maxElement = 0;
        for (size_t i: indices) {
            if (i > maxElement) {
                maxElement = i;
            }
        }
        for (size_t i = 0; i < concatedBatch.size(); ++i) {
            Y_ENSURE_EX(
                batchFeatures[i].size() > maxElement,
                NNeuralRanker::TConcatException()
                    << "slice \"" << slice.GetName()
                    << "\" should be longer than " << maxElement
                    << ", but has size = " << batchFeatures[i].size()
            );
            for (int index: indices) {
                concatedBatch[i].push_back(batchFeatures[i][index]);
            }
        }
    }

    void AppendBatchEmbedding(
        const NNeuralRanker::NProto::TEmbedding& embedding,
        const TBatch& batchFeatures,
        TVector<TVector<float>>& concatedBatch)
    {
        size_t length = embedding.GetLen();

        for (size_t i = 0; i < concatedBatch.size(); ++i) {
            // It is a rare situation: approximately 0.26% of all documents
            // lack some embeddings. In that case they are filled with zeros.
            if (batchFeatures[i].empty()) {
                concatedBatch[i].insert(concatedBatch[i].end(), length, .0f);
                continue;
            }

            Y_ENSURE_EX(
                batchFeatures[i].size() == length,
                NNeuralRanker::TConcatException()
                    << "embedding \"" << embedding.GetName()
                    << "\" should be length " << length
                    << ", but has size = " << batchFeatures[i].size()
            );

            concatedBatch[i].insert(
                concatedBatch[i].end(),
                batchFeatures[i].begin(),
                batchFeatures[i].end()
            );
        }
    }
}

size_t NNeuralRanker::TFeaturesConcater::GetRawVectorLen(
    const NProto::TInput& input)
{
    size_t currentSize = 0;
    for (size_t i = 0; i < input.InputPartsSize(); ++i) {
        const NProto::TInputPart& currentInputPart = input.GetInputParts(i);
        if (currentInputPart.HasSlice()) {
            const NProto::TSlice& slice = currentInputPart.GetSlice();
            Y_ENSURE_EX(
                slice.HasName() && (!slice.GetName().empty())
                    && (slice.IndicesSize() > 0),
                TConcatException() << "invalid slice given: " << slice
            );
            currentSize += slice.IndicesSize();
        } else if (currentInputPart.HasEmbedding()) {
            const NProto::TEmbedding& embedding = currentInputPart.GetEmbedding();
            Y_ENSURE_EX(
                embedding.HasName() && (!embedding.GetName().empty())
                    && embedding.HasLen() && (embedding.GetLen() > 0),
                TConcatException() << "invalid embedding given: " << embedding
            );
            currentSize += embedding.GetLen();
        } else {
            throw TConcatException() << "neither slice nor embedding is given: " << currentInputPart;
        }
    }
    return currentSize;
}

NNeuralRanker::TFeaturesConcater::TFeaturesConcater(const NProto::TInput& input) {
    ConcatedLen = GetRawVectorLen(input);
    Input = input;
}

TVector<float> NNeuralRanker::TFeaturesConcater::Concat(const TInputMap& features) const {
    TInputBatchMap batch;
    for (auto&& [name, featuresArray]: features) {
        batch[name] = {featuresArray};
    }
    return Concat(batch)[0];
}

TVector<TVector<float>> NNeuralRanker::TFeaturesConcater::Concat(
    const TInputBatchMap& batchFeatures) const
{
    Y_ENSURE_EX(
        batchFeatures.size() > 0,
        TConcatException() << "batch map of features is empty"
    );
    auto firstPair = *batchFeatures.begin();
    size_t batchSize = firstPair.second.size();

    for (const auto& i: batchFeatures) {
        Y_ENSURE_EX(
            i.second.size() == batchSize,
            TConcatException() << "batchsizes should be equal, "
                         << "have batchsize " << i.second.size()
                         << " for \"" << i.first
                         << "\", but batchsize " << batchSize
                         << " for \"" << firstPair.first << "\""
        );
    }
    TVector<TVector<float>> featuresConcated;
    featuresConcated.resize(batchSize);
    for (auto& featuresVector: featuresConcated) {
        featuresVector.reserve(ConcatedLen);
    }

    for (size_t i = 0; i < Input.InputPartsSize(); ++i) {
        const auto &currentInputPart = Input.GetInputParts(i);
        if (currentInputPart.HasSlice()) {
            const NProto::TSlice& slice = currentInputPart.GetSlice();
            TString name = slice.GetName();
            Y_ENSURE_EX(
                batchFeatures.contains(name),
                TConcatException() << "slice \"" << name << "\" is not given"
            );
            AppendBatchSlice(slice, batchFeatures.at(name), featuresConcated);
        } else {
            const NProto::TEmbedding& embedding = currentInputPart.GetEmbedding();
            TString name = embedding.GetName();
            Y_ENSURE_EX(
                batchFeatures.contains(name),
                TConcatException() << "embedding \"" << name << "\" is not given"
            );
            AppendBatchEmbedding(embedding, batchFeatures.at(name), featuresConcated);
        }
    }
    for (auto& featuresVector: featuresConcated) {
        Y_ASSERT(featuresVector.size() == ConcatedLen);
    }
    return featuresConcated;
}
