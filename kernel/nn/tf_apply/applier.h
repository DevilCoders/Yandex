#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/bt_exception.h>
#include <util/generic/deque.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <tensorflow/core/public/session.h>
#include <tensorflow/core/graph/default_device.h>

#include "utils.h"
#include "inputs.h"

namespace NTFModel {
    using TTFEnv = tensorflow::Env;
    using TTFGraphDef = tensorflow::GraphDef;
    using TTFSession = tensorflow::Session;
    using TTFSessionOptions = tensorflow::SessionOptions;
    using TTFStatus = tensorflow::Status;

    using std::pair;

    struct TTFApplierResult {
        TVector<TTFTensor> Tensors;
        TVector<TArrayRef<float>> Data;
    };

    TVector<TTFTensor> Calculate(
        TTFSession* sessionPtr,
        const TVector<pair<TString, TTFTensor>>& inputs,
        const TVector<TString>& outputNames);

    void EnsureValidTensorflowStatus(const TTFStatus& status, TStringBuf msg = "");
    TTFGraphDef ReadModelProtobuf(const TString& graphPath);
    TTFSessionOptions GetDefaultSessionOptions();
    THolder<TTFSession> InitSessionWithProtobuf(const TTFGraphDef& graphDef, TTFSessionOptions options = GetDefaultSessionOptions());
    THolder<TTFSession> InitSessionWithGraph(const TString& graphPath, TTFSessionOptions options = GetDefaultSessionOptions());

    class TTFVanillaApplier {
    public:
        TTFVanillaApplier(
            const TString& graphDefPath,
            const TVector<TString>& modelOutputNames,
            TTFSessionOptions options = GetDefaultSessionOptions());

        TTFVanillaApplier(
            const TTFGraphDef& graphDef,
            const TVector<TString>& modelOutputNames,
            TTFSessionOptions options = GetDefaultSessionOptions());

        TTFVanillaApplier(
            THolder<TTFSession> session,
            const TVector<TString>& modelOutputNames);

        ~TTFVanillaApplier();

        TVector<TTFTensor> Run(const TVector<pair<TString, TTFTensor>>& inputs) const;

    private:
        THolder<TTFSession> SessionPtr;
        TVector<TString> ModelOutputNames;
    };

    class TTFDynamicApplier {
    public:
        TTFDynamicApplier(
            const TString& graphDefPath,
            TTFInt outputVectorLen,
            const TString& modelOutputName, // only one output supported
            TTFSessionOptions options = GetDefaultSessionOptions());

        TTFDynamicApplier(
            const TTFGraphDef& graphDef,
            TTFInt outputVectorLen,
            const TString& modelOutputName, // only one output supported
            TTFSessionOptions options = GetDefaultSessionOptions());

        TTFDynamicApplier(
            THolder<TTFSession> session,
            TTFInt outputVectorLen,
            const TString& modelOutputName); // only one output supported

        TTFApplierResult Run(ITFInputNode* inputNode) const;

    private:
        TTFVanillaApplier VanillaApplier;
        TTFInt OutputVectorLen = 0;
    };

    template <typename... TInTypes>
    class TTFApplier {
    public:
        using TInputSignature = TTFInputSignature<TInTypes...>;

        TTFApplier(
            const TString& graphDefPath,
            TTFInt outputVectorLen,
            const TString& modelOutputName, // only one output supported
            TTFSessionOptions options = GetDefaultSessionOptions())
            : Impl(graphDefPath, outputVectorLen, modelOutputName, options)
        {
        }

        TTFApplier(
            const TTFGraphDef& graphDef,
            TTFInt outputVectorLen,
            const TString& modelOutputName, // only one output supported
            TTFSessionOptions options = GetDefaultSessionOptions())
            : Impl(graphDef, outputVectorLen, modelOutputName, options)
        {
        }

        TTFApplier(
            THolder<TTFSession> session,
            TTFInt outputVectorLen,
            const TString& modelOutputName) // only one output supported
            : Impl(std::move(session), outputVectorLen, modelOutputName)
        {
        }

        TTFApplierResult Run(TTFInputNode<TInTypes...>* inputNode) const {
            return Impl.Run(inputNode);
        }

    private:
        TTFDynamicApplier Impl;
    };
}
