#pragma once

#include <library/cpp/float16/float16.h>

#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>

#include <algorithm>

namespace NBertApplier {

//! Bert model input data, packed for batch processing
template <typename TFloatType>
struct TBertPackedInput {
    size_t MaxInputLength;
    TVector<int> Inputs;
    TVector<int> InputLengths;
    TMaybe<TVector<int>> SegmentIds;

    TMaybe<TVector<TFloatType>> SplitBertL3Embeddings;
    TMaybe<TVector<int>> SplitBertL3Mask;
};

//! Bert model input data
class TBertInput {
private:
    size_t MaxInputLength_;
    TVector<TVector<int>> Inputs_;
    TVector<int> InputLengths_;
    TMaybe<TVector<TVector<int>>> SegmentIds_;

    TMaybe<TVector<TVector<float>>> SplitBertL3EmbeddingsFP32_;
    TMaybe<TVector<TVector<TFloat16>>> SplitBertL3EmbeddingsFP16_;
    TMaybe<TVector<TVector<int>>> SplitBertL3Mask_;

    friend void swap(TBertInput& l, TBertInput& r) noexcept;

public:
    //! The BERT model inputs
    /** @param expectedbatchSize - The expected size of the batch. Used only for a memory reservation
        @param useSegmentIds - If explicit segment ids should be used
    */
    TBertInput(size_t expectedBatchSize = 32, size_t maxInputLength = 128, bool useSegmentIds = false);

    //! Add an input
    /** @param data - The input data. It should not include the BOS marker.
    */
    void Add(TArrayRef<int const> data,
             const TMaybe<TArrayRef<int const>>& segmentIds = Nothing());

    //! Add mask for Split BERT L3 level
    void AddForSplitBertL3Mask(TArrayRef<i32 const> splitQueryPartInputMask,
                               TArrayRef<i32 const> splitDocPartInputMask,
                               size_t               splitQueryLength,
                               size_t               splitDocLength);

    //! Add embedding for Split BERT L3 level
    template <typename TFloatType>
    void AddForSplitBertL3Embedding(TArrayRef<TFloatType const> splitQueryPartEmbedding,
                                    TArrayRef<TFloatType const> splitDocPartEmbedding,
                                    size_t splitQueryLength,
                                    size_t splitDocLength) {
        TMaybe<TVector<TVector<TFloatType>>>& splitBertL3Embeddings = SplitBertL3Embeddings<TFloatType>();
        if (!splitBertL3Embeddings.Defined()) {
            splitBertL3Embeddings = TVector<TVector<TFloatType>>{};
        }
        Y_ASSERT((splitQueryLength > 0) == (splitDocLength > 0));

        TVector<TFloatType>& newEmbedding = splitBertL3Embeddings->emplace_back();

        if (splitQueryLength > 0) {
            const size_t queryPartEmbeddingSize = splitQueryPartEmbedding.size() / splitQueryLength;
            const size_t docPartEmbeddingSize = splitDocPartEmbedding.size() / splitDocLength;
            const size_t embeddingSize = std::max(queryPartEmbeddingSize, docPartEmbeddingSize);

            Append(splitQueryPartEmbedding, queryPartEmbeddingSize, embeddingSize, newEmbedding);
            Append(splitDocPartEmbedding, docPartEmbeddingSize, embeddingSize, newEmbedding);
        } else {
            newEmbedding.insert(newEmbedding.end(), splitQueryPartEmbedding.begin(), splitQueryPartEmbedding.end());
            newEmbedding.insert(newEmbedding.end(), splitDocPartEmbedding.begin(), splitDocPartEmbedding.end());
        }
    }

    template <typename TFloatType>
    TBertPackedInput<TFloatType> PackBatch() const {
        constexpr int Padding = 0;
        constexpr int PaddingSegment = 0;

        TBertPackedInput<TFloatType> packedBatch;

        size_t numInputs = InputLengths_.size();
        auto maxInputLength = *std::max_element(InputLengths_.begin(), InputLengths_.end());

        packedBatch.MaxInputLength = maxInputLength;
        packedBatch.InputLengths = InputLengths_;
        // 0. Pack input sequences and segment ids, if they exist
        if (!Inputs_.empty()) {
            packedBatch.Inputs.reserve(numInputs * maxInputLength);
            for (size_t i = 0; i < numInputs; ++i) {
                size_t paddingLength = maxInputLength - Inputs_[i].size();
                packedBatch.Inputs.insert(packedBatch.Inputs.end(),
                                          Inputs_[i].begin(),
                                          Inputs_[i].end());
                packedBatch.Inputs.insert(packedBatch.Inputs.end(),
                                          paddingLength,
                                          Padding);
            }

            // 1. Pack segments, if they exist
            if (SegmentIds_.Defined() && !SegmentIds_->empty()) {
                packedBatch.SegmentIds.ConstructInPlace(Reserve(numInputs * maxInputLength));
                for (size_t i = 0; i < numInputs; ++i) {
                    size_t paddingLength = maxInputLength - (*SegmentIds_)[i].size();
                    packedBatch.SegmentIds->insert(packedBatch.SegmentIds->end(),
                                                   (*SegmentIds_)[i].begin(),
                                                   (*SegmentIds_)[i].end());
                    packedBatch.SegmentIds->insert(packedBatch.SegmentIds->end(),
                                                   paddingLength,
                                                   PaddingSegment);
                }
            }
        }

        // 2. Pack parts for split BERT l3
        //    Note: in that case we must check that all inputs have the same length
        auto splitBertL3Embeddings = SplitBertL3Embeddings<TFloatType>();
        if (splitBertL3Embeddings.Defined()) {
            Y_ENSURE(SplitBertL3Mask_.Defined());
            PackL3DataToBatch(*splitBertL3Embeddings, numInputs, packedBatch.SplitBertL3Embeddings);
            PackL3DataToBatch(*SplitBertL3Mask_, numInputs, packedBatch.SplitBertL3Mask);
        }
        return packedBatch;
    }

private:
    template <typename TFloatType>
    void Append(TArrayRef<TFloatType const> fromVector, size_t fromVectorStride, size_t toVectorStride, TVector<TFloatType>& toVector) {
        Y_ASSERT(fromVector.size() % fromVectorStride == 0);
        Y_ASSERT(toVector.size() % toVectorStride == 0);
        Y_ASSERT(fromVectorStride <= toVectorStride);

        if (fromVectorStride == toVectorStride) {
            toVector.insert(toVector.end(), fromVector.begin(), fromVector.end());
            return;
        }

        toVector.reserve(toVector.size() + fromVector.size() * toVectorStride / fromVectorStride);
        for (size_t i = 0; i < fromVector.size(); i += fromVectorStride) {
            toVector.insert(toVector.end(), fromVector.begin() + i, fromVector.begin() + i + fromVectorStride);
            toVector.resize(toVector.size() + toVectorStride - fromVectorStride, 0);
        }
    }

    template <typename TType>
    void PackL3DataToBatch(const TVector<TVector<TType>>& l3OriginalDataArray,
                           size_t numInputs,
                           TMaybe<TVector<TType>>& l3PackedData) const {
        Y_ENSURE(numInputs == l3OriginalDataArray.size());
        auto lengthPerInput = l3OriginalDataArray.front().size();
        l3PackedData.ConstructInPlace(Reserve(numInputs * lengthPerInput));
        for (const auto& l3Input : l3OriginalDataArray) {
            Y_ENSURE(l3Input.size() == lengthPerInput);
            l3PackedData->insert(l3PackedData->end(), l3Input.begin(), l3Input.end());
        }
    }

    template <typename TFloatType>
    const TMaybe<TVector<TVector<TFloatType>>>& SplitBertL3Embeddings() const noexcept {
        if constexpr (std::is_same_v<TFloatType, float>) {
            return SplitBertL3EmbeddingsFP32_;
        }
        if constexpr(std::is_same_v<TFloatType, TFloat16>) {
            return SplitBertL3EmbeddingsFP16_;
        }
    }

    template <typename TFloatType>
    TMaybe<TVector<TVector<TFloatType>>>& SplitBertL3Embeddings() noexcept {
        if constexpr (std::is_same_v<TFloatType, float>) {
            return SplitBertL3EmbeddingsFP32_;
        }
        if constexpr(std::is_same_v<TFloatType, TFloat16>) {
            return SplitBertL3EmbeddingsFP16_;
        }
    }
};

void swap(TBertInput& l, TBertInput& r) noexcept;

//! Bert model interface
template <typename TModelResult>
class IBertModel {
public:
    using TResult = TModelResult;
public:
    //! Processes the batch
    /** Number of inputs should not exceed the MaxBatchSize. Length of each input should not exceed the MaxInputLength
        @param input - The batch
    */
    virtual TResult ProcessBatch(const TBertInput& input) = 0;
    virtual void PrepareBackend() = 0;
    virtual size_t GetMaxBatchSize() const = 0;
    virtual size_t GetMaxInputLength() const = 0;

    virtual ~IBertModel() = default;
};

}
