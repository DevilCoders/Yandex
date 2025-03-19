#include <util/generic/array_ref.h>

#include <dict/mt/libs/nn/ynmt/extra/encoder_head.h>
#include <kernel/bert/bert_wrapper.h>

using NDict::NMT::NYNMT::TClassificationHead;
using NBertApplier::TBertWrapper;

namespace NBertHighlighter {
    template <typename TFloatType>
    class TBertHighlighter {
    public:
        TBertHighlighter(int maxBatchSize, int maxInputLength, const TString& modelPath, NDict::NMT::NYNMT::TBackendPtr backend);

        // Already tokenized inputs are expected here.
        TVector<TFloatType> ProcessBatch(const TConstArrayRef<TVector<int>>& batch);
        int GetMaxInputLength();
        size_t GetOutputSize();
    private:
        int MaxInputLength;
        int MaxBatchSize;
        TBlob ModelBlob;
        NDict::NMT::NYNMT::TBackendWithMemory Backend;
        NDict::NMT::NYNMT::TInitializationEnvironment InitEnv;
        TBertWrapper<TFloatType, TClassificationHead> Model;
    };
} // namespace NBertHighlighter
