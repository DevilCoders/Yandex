#pragma once

#include <kernel/nn/neural_ranker/protos/meta_info/meta_info.pb.h>
#include <util/generic/array_ref.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NNeuralRanker {
    namespace NProto {
        using namespace NNeuralRankerProtocol;
    }

    class TConcatException : public yexception {};

    class TFeaturesConcater {
    public:
        using TBatch = TVector<TConstArrayRef<float>>;
        using TInputMap = THashMap<TString, TConstArrayRef<float>>;
        using TInputBatchMap = THashMap<TString, TBatch>;

        TFeaturesConcater(const NProto::TInput& input);

        TVector<float> Concat(const TInputMap& features) const;
        TVector<TVector<float>> Concat(const TInputBatchMap& batchFeatures) const;

        static size_t GetRawVectorLen(const NProto::TInput &input);

    private:
        NProto::TInput Input;
        size_t ConcatedLen = 0;
    };
}
