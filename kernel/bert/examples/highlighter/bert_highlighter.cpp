#include "bert_highlighter.h"

using NDict::NMT::NYNMT::TClassificationHead;

namespace NBertHighlighter {
    template <typename TFloatType>
    TBertHighlighter<TFloatType>::TBertHighlighter(int maxBatchSize, int maxInputLength, const TString& modelPath, NDict::NMT::NYNMT::TBackendPtr backend)
        : MaxInputLength(maxInputLength)
        , MaxBatchSize(maxBatchSize)
        , ModelBlob(TBlob::FromFile(modelPath))
        , Backend(backend)
        , InitEnv(Backend.Backend.Get(), {}, false, false)
        , Model(maxBatchSize, maxInputLength, 0) {
        auto head = MakeHolder<TClassificationHead<TFloatType>>(
            maxBatchSize,
            InitEnv,
            NDict::NMT::NYNMT::ReadModelProtoWithMeta(ModelBlob)
        );
        Model.Initialize(ModelBlob, InitEnv, Backend, std::move(head));
    }

    template <typename TFloatType>
    TVector<TFloatType> TBertHighlighter<TFloatType>::ProcessBatch(const TConstArrayRef<TVector<int>>& batch) {
        Y_ENSURE(
            batch.size() <= static_cast<size_t>(MaxBatchSize),
            "MaxBatchSize exceeded: got " << batch.size() << " with MaxBatchSize = " << MaxBatchSize
        );

        NBertApplier::TBertInput bertInput(batch.size(), MaxInputLength);
        for (const auto& sample : batch)
            bertInput.Add(sample);

        return Model.ProcessBatch(bertInput);
    }

    template <typename TFloatType>
    int TBertHighlighter<TFloatType>::GetMaxInputLength() {
        return MaxInputLength;
    }

    template <typename TFloatType>
    size_t TBertHighlighter<TFloatType>::GetOutputSize() {
        return static_cast<size_t>(Model.GetHead().GetOutputLayer().OutSize);
    }
} // namespace NBertHighlighter

template class NBertHighlighter::TBertHighlighter<TFloat16>;
template class NBertHighlighter::TBertHighlighter<float>;
