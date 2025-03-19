#pragma once

#include "bert_interface.h"
#include "bert_wrapper.h"

#include <dict/mt/libs/nn/ynmt/backend.h>
#include <dict/mt/libs/nn/ynmt/models/transformer_encoder.h>
#include <dict/mt/libs/nn/ynmt/config_helper/model_reader.h>
#include <dict/mt/libs/nn/ynmt_backend/cpu/backend.h>

#include <util/memory/blob.h>

namespace NBertApplier {

template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
class TBertModel final : public IBertModel<typename TEncoderHead<TFloatType>::TResult> {
private:
    const size_t MaxBatchSize;
    const size_t MaxInputLength;
    NDict::NMT::NYNMT::TBackendWithMemory Backend;
    TBertWrapper<TFloatType, TEncoderHead> Model;
public:
    using TResult = typename IBertModel<typename TEncoderHead<TFloatType>::TResult>::TResult;
    //! Creates the BERT model
    /** @param modelPath - Path to the model file
        @param maxBatchSize - Maximum inputs in a batch
        @param maxInputLength - Maximum length of a single input including the mandatory "beginning of stream (BOS)" marker
    */
    TBertModel(TString const& modelPath, size_t maxBatchSize = 32, size_t maxInputLength = 128
                , NDict::NMT::NYNMT::TBackendPtr backend = nullptr)
        : MaxBatchSize(maxBatchSize)
        , MaxInputLength(maxInputLength)
        , Backend(backend != nullptr ? std::move(backend) : MakeIntrusive<NDict::NMT::NYNMT::TCpuBackend>())
        , Model(maxBatchSize, maxInputLength)
    {
        LoadModel(modelPath);
    }

    //! Creates the BERT model
    /** @param params - Blob with the model parameters
        @param maxBatchSize - Maximum inputs in a batch
        @param maxInputLength - Maximum length of a single input including the mandatory "beginning of stream (BOS)" marker
    */
    TBertModel(TBlob const& params, size_t maxBatchSize = 32, size_t maxInputLength = 128
                , NDict::NMT::NYNMT::TBackendPtr backend = nullptr)
        : MaxBatchSize(maxBatchSize)
        , MaxInputLength(maxInputLength)
        , Backend(backend != nullptr ? std::move(backend) : MakeIntrusive<NDict::NMT::NYNMT::TCpuBackend>())
        , Model(maxBatchSize, maxInputLength)
    {
        LoadModel(params);
    }
    ~TBertModel() = default;

    TResult ProcessBatch(const TBertInput& input) override {
        if (!Model.IsSessionInitialized()) {
            Model.InitSession(Backend);
        }
        return Model.ProcessBatch(input);
    }

    void PrepareBackend() override {
        Backend.Backend->Prepare();
    }

    size_t GetMaxBatchSize() const override {
        return MaxBatchSize;
    }

    size_t GetMaxInputLength() const override {
        return MaxInputLength;
    }

private:
    void LoadModel(const TBlob& params) {
        using namespace NDict::NMT::NYNMT;

        TInitializationEnvironment initEnv(Backend.Backend.Get(), {params}, false, false);

        // get size of each embedding
        auto proto = ReadModelProtoWithMeta(params);

        // initialize bert
        Model.LoadModel(params, initEnv, MakeHolder<TEncoderHead<TFloatType>>(MaxBatchSize, initEnv, proto));
    }

    void LoadModel(const TString& path) {
        LoadModel(TBlob::FromFile(path));
    }
};

}
