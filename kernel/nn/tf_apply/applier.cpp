#include "applier.h"

void NTFModel::EnsureValidTensorflowStatus(const TTFStatus& status, TStringBuf msg) {
    Y_ENSURE(status.ok(), (msg.empty() ? "" : TString{msg} + ": ") + status.ToString());
}

NTFModel::TTFGraphDef NTFModel::ReadModelProtobuf(const TString& graphPath) {
    TTFGraphDef graphDef;
    EnsureValidTensorflowStatus(
        ReadBinaryProto(TTFEnv::Default(), graphPath, &graphDef),
        "reading binary proto for graph");
    return graphDef;
}

NTFModel::TTFSessionOptions NTFModel::GetDefaultSessionOptions() {
    TTFSessionOptions options;
    options.config.set_log_device_placement(false);
    options.config.set_allow_soft_placement(true);
    options.config.set_inter_op_parallelism_threads(1);
    options.config.set_intra_op_parallelism_threads(1);
    options.config.mutable_arcadia_specific_config_proto()->set_enable_mkldnn_optimization(true);
    options.config.mutable_arcadia_specific_config_proto()->set_mkldnn_use_weights_as_given(true);
    return options;
}

THolder<NTFModel::TTFSession> NTFModel::InitSessionWithProtobuf(const TTFGraphDef& graphDef, TTFSessionOptions options) {
    TTFSession* sessionTmpPtr = nullptr;
    NTFModel::EnsureValidTensorflowStatus(NewSession(options, &sessionTmpPtr), "creating session");
    THolder<TTFSession> sessionPtr(sessionTmpPtr);
    NTFModel::EnsureValidTensorflowStatus(sessionPtr->Create(graphDef), "creating graph for session from graphDef");
    return sessionPtr;
}

THolder<NTFModel::TTFSession> NTFModel::InitSessionWithGraph(const TString& graphPath, TTFSessionOptions options) {
    TTFGraphDef graphDef = ReadModelProtobuf(graphPath);
    return InitSessionWithProtobuf(graphDef, options);
}

TVector<NTFModel::TTFTensor> NTFModel::Calculate(
    TTFSession* session,
    const TVector<NTFModel::pair<TString, NTFModel::TTFTensor>>& inputs,
    const TVector<TString>& outputNames)
{
    Y_ENSURE_EX(session,
        TTFException() << "pointer to TTFSession is nullptr");

    TVector<NTFModel::TTFTensor> outputTensors;
    outputTensors.reserve(outputNames.size());

    NTFModel::EnsureValidTensorflowStatus(
        session->Run(inputs, outputNames, /*targetNodeNames*/ {}, &outputTensors),
        "session run for calculate"
    );
    return outputTensors;
}

NTFModel::TTFVanillaApplier::TTFVanillaApplier(
    const TString& graphDefPath,
    const TVector<TString>& modelOutputNames, // only one output supported
    TTFSessionOptions options)
    : SessionPtr(NTFModel::InitSessionWithGraph(graphDefPath, options))
    , ModelOutputNames(modelOutputNames)
{
}

NTFModel::TTFVanillaApplier::TTFVanillaApplier(
    const TTFGraphDef& graphDef,
    const TVector<TString>& modelOutputNames, // only one output supported
    TTFSessionOptions options)
    : SessionPtr(NTFModel::InitSessionWithProtobuf(graphDef, options))
    , ModelOutputNames(modelOutputNames)
{
}

NTFModel::TTFVanillaApplier::TTFVanillaApplier(
    THolder<NTFModel::TTFSession> session,
    const TVector<TString>& modelOutputNames) // only one output supported
    : SessionPtr(std::move(session))
    , ModelOutputNames(modelOutputNames)
{
}

NTFModel::TTFVanillaApplier::~TTFVanillaApplier() {
    NTFModel::EnsureValidTensorflowStatus(SessionPtr->Close(), "closing session");
}

TVector<NTFModel::TTFTensor> NTFModel::TTFVanillaApplier::Run(const TVector<pair<TString, TTFTensor>>& inputs) const {
    return NTFModel::Calculate(SessionPtr.Get(), inputs, ModelOutputNames);
}

NTFModel::TTFDynamicApplier::TTFDynamicApplier(
    const TString& graphDefPath,
    TTFInt outputVectorLen,
    const TString& modelOutputName, // only one output supported
    TTFSessionOptions options)
    : VanillaApplier(graphDefPath, {modelOutputName}, options)
    , OutputVectorLen(outputVectorLen)
{
}

NTFModel::TTFDynamicApplier::TTFDynamicApplier(
    const TTFGraphDef& graphDef,
    TTFInt outputVectorLen,
    const TString& modelOutputName, // only one output supported
    TTFSessionOptions options)
    : VanillaApplier(graphDef, {modelOutputName}, options)
    , OutputVectorLen(outputVectorLen)
{
}

NTFModel::TTFDynamicApplier::TTFDynamicApplier(
    THolder<NTFModel::TTFSession> session,
    TTFInt outputVectorLen,
    const TString& modelOutputName) // only one output supported
    : VanillaApplier(std::move(session), {modelOutputName})
    , OutputVectorLen(outputVectorLen)
{
}

NTFModel::TTFApplierResult NTFModel::TTFDynamicApplier::Run(ITFInputNode* inputNode) const {
    TTFApplierResult output;
    Y_ENSURE_EX(
        inputNode != nullptr,
        TTFException() << "given ITFInputNode is nullptr"
    );
    while (inputNode->Valid()) {
        TVector<pair<TString, TTFTensor>> modelInputs = inputNode->Get();
        inputNode->Next();
        TVector<TTFTensor> tfOutputs = VanillaApplier.Run(modelInputs);
        Y_VERIFY(tfOutputs.size() == 1);

        TTFTensor tfOutput = tfOutputs[0];

        Y_VERIFY(
            ((tfOutput.dims() == 1) && (OutputVectorLen == 1)) ||
            ((tfOutput.dims() == 2) && (tfOutput.dim_size(1) == OutputVectorLen))
        );

        output.Tensors.push_back(tfOutput);
        float* predictionData = tfOutput.flat<float>().data();
        TTFInt numCurSamples = tfOutput.dim_size(0);
        TTFInt numOldSamples = output.Data.size();
        output.Data.resize(numOldSamples + numCurSamples);
        for (TTFInt j = 0; j < numCurSamples; ++j) {
            output.Data[numOldSamples + j] = MakeArrayRef(predictionData + j * OutputVectorLen, OutputVectorLen);
        }
    }
    return output;
}
