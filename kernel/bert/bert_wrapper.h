#pragma once

#include "bert_interface.h"

#include <dict/mt/libs/nn/ynmt/models/transformer_encoder.h>

#include <dict/mt/libs/nn/ynmt/config_helper/model_reader.h>
#include <dict/mt/libs/nn/ynmt/config_helper/models/transformer_encoder.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>
#include <util/system/tls.h>


namespace NBertApplier {
    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    class TBertWrapper {
    public:
        using THead = TEncoderHead<TFloatType>;

        //! Creates the BERT wrapper
        /** @param maxBatchSize - Maximum inputs in a batch
            @param maxInputLength - Maximum length of a single input including the mandatory "beginning of stream (BOS)" marker
            @padToken - The token for padding and BOS
        */
        TBertWrapper(size_t maxBatchSize = 32, size_t maxInputLength = 128, int padToken = 0);

        void Initialize(
            const TBlob& modelBlob,
            NDict::NMT::NYNMT::TInitializationEnvironment& initEnv,
            NDict::NMT::NYNMT::TBackendWithMemory& backend,
            THolder<THead> head);

        void InitSession(NDict::NMT::NYNMT::TBackendWithMemory& backend);
        void InitSession(NDict::NMT::NYNMT::TBackendPtr backend);

        bool IsSessionInitialized() const;

        typename THead::TResult ProcessBatch(const TBertInput& input);

        const TEncoderHead<TFloatType>& GetHead() const {
            return Model->GetHead();
        }

        void LoadModel(
            const TBlob& modelBlob,
            NDict::NMT::NYNMT::TInitializationEnvironment& initEnv,
            THolder<THead> head);

    private:
        TBertWrapper(TBertWrapper const&) = delete;
        TBertWrapper& operator=(TBertWrapper const&) = delete;

    private:
        THolder<NDict::NMT::NYNMT::TDeviceMemory> ModelMemory;
        Y_THREAD(THolder<typename NDict::NMT::NYNMT::TTransformerEncoderModelBase<TFloatType>::TSessionData>) Session;
        Y_THREAD(THolder<NDict::NMT::NYNMT::TBackendWithMemory>) ThreadBacked;
        typename THead::TParams HeadParams;
        THolder<NDict::NMT::NYNMT::TTransformerEncoderModel<TFloatType, TEncoderHead>> Model;

        const size_t MaxBatchSize;
        const size_t MaxInputLength;
        const int PadToken;
    };

    template <typename TFloatType, template <typename> typename TEncoderHead>
    TBertWrapper<TFloatType, TEncoderHead>::TBertWrapper(size_t maxBatchSize, size_t maxInputLength, int padToken)
        : MaxBatchSize(maxBatchSize)
        , MaxInputLength(maxInputLength)
        , PadToken(padToken)
    {
        Y_ENSURE(maxBatchSize > 0, "Maximum batch size should be positive");
        Y_ENSURE(maxInputLength > 0, "Maximum input length should be positive");
    }

    template <typename TFloatType, template <typename> typename TEncoderHead>
    void TBertWrapper<TFloatType, TEncoderHead>::Initialize(
        const TBlob& modelBlob,
        NDict::NMT::NYNMT::TInitializationEnvironment& initEnv,
        NDict::NMT::NYNMT::TBackendWithMemory& backend,
        THolder<THead> head)
    {
        LoadModel(modelBlob, initEnv, std::move(head));
        InitSession(backend);
    }

    template <typename TFloatType, template <typename> typename TEncoderHead>
    void TBertWrapper<TFloatType, TEncoderHead>::InitSession(NDict::NMT::NYNMT::TBackendWithMemory& backend) {
        Session.Get() = THolder(
            Model->CreateSession(
                     backend.SessionsScratchMemory[0].get(),
                     ModelMemory->GetStream()->CreateStream())
                .release());
        Session->Reserve();
        Y_IF_DEBUG(Session->ReportUsage());
    }

    template <typename TFloatType, template <typename> typename TEncoderHead>
    void TBertWrapper<TFloatType, TEncoderHead>::InitSession(NDict::NMT::NYNMT::TBackendPtr backend) {
        ThreadBacked = new NDict::NMT::NYNMT::TBackendWithMemory(backend);
        InitSession(*ThreadBacked.Get());
    }

    template <typename TFloatType, template <typename> typename TEncoderHead>
    bool TBertWrapper<TFloatType, TEncoderHead>::IsSessionInitialized() const {
        return Session.Get().Get();
    }

    template <typename TFloatType, template <typename> typename TEncoderHead>
    typename TEncoderHead<TFloatType>::TResult TBertWrapper<TFloatType, TEncoderHead>::ProcessBatch(const TBertInput& batch) {
        const auto packedBatch = batch.PackBatch<TFloatType>();

        Y_ENSURE(IsSessionInitialized());
        Y_ENSURE(packedBatch.InputLengths.size() <= MaxBatchSize, "Maximum batch size exceeded");
        Y_ENSURE(packedBatch.MaxInputLength <= MaxInputLength, "Batch MaxInputLength is incompatible with model MaxInputLength");

        auto result = Model->EncodeBatch(
            Session.Get().Get(),
            packedBatch.InputLengths.size(),
            packedBatch.MaxInputLength,
            packedBatch.Inputs.data(),
            packedBatch.InputLengths.data(),
            Nothing(),
            packedBatch.SegmentIds,
            HeadParams,
            packedBatch.SplitBertL3Embeddings,
            packedBatch.SplitBertL3Mask);
        return result;
    }

    template <typename TFloatType, template <typename> typename TEncoderHead>
    void TBertWrapper<TFloatType, TEncoderHead>::LoadModel(
        const TBlob& modelBlob,
        NDict::NMT::NYNMT::TInitializationEnvironment& initEnv,
        THolder<THead> head)
    {
        auto proto = NDict::NMT::NYNMT::ReadModelProtoWithMeta(TNpzFile(modelBlob, initEnv.ConvertFloat16ToFloat32));

        auto params = NDict::NMT::NYNMT::ParseTransformerEncoderParams(proto.Meta);
        params.MaxInpLen = MaxInputLength;
        params.MaxBatchSize = MaxBatchSize;

        Model = MakeHolder<NDict::NMT::NYNMT::TTransformerEncoderModel<TFloatType, TEncoderHead>>(
            initEnv,
            proto,
            params,
            std::unique_ptr<THead>(head.Release()));

        ModelMemory.Reset(initEnv.Memory.release());
    }
}
