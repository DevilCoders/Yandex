#include "configs_checks.h"
#include "format_version.h"
#include "optimized_model_applier.h"

#include <kernel/dssm_applier/utils/utils.h>

#include <kernel/factors_selector/factors.h>
#include <kernel/searchlog/errorlog.h>

#include <library/cpp/protobuf/yt/proto2yt.h>
#include <library/cpp/protobuf/yt/yt2proto.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/yson/node/node_io.h>

#include <util/string/split.h>

#include <algorithm>

namespace {

    auto MakeSamplePtr(const NOptimizedModel::TVariablesInput& input) {
        TVector<TString> headerFields;
        TVector<TString> headerValues;
        for (const auto& [headerField, headerValue] : input.Headers) {
            headerFields.emplace_back(headerField);
            headerValues.emplace_back(headerValue);
        }
        return MakeAtomicShared<NNeuralNetApplier::TSample>(headerFields, headerValues);
    }

    TVector<TAtomicSharedPtr<NNeuralNetApplier::ISample>> MakeSamplesPtrs(
        TConstArrayRef<NOptimizedModel::TVariablesInput> inputs)
    {
        TVector<TAtomicSharedPtr<NNeuralNetApplier::ISample>> samples;
        samples.reserve(inputs.size());
        for (const NOptimizedModel::TVariablesInput& input : inputs) {
            samples.emplace_back(MakeSamplePtr(input));
        }
        return samples;
    }

    auto MakeNameToVersion(const TString& name, const TString& version) {
        NOptimizedModel::NProto::TNameToVersion result;
        result.SetName(name);
        result.SetVersion(version);
        return result;
    };

    template <typename T>
    const T* FindPtrFromMapWithNameAndVersion(
        const THashMap<TString, THashMap<TString, T>>& nameToVersionToValue,
        const TStringBuf name,
        const TStringBuf version)
    {
        if (auto* versionToValuePtr = nameToVersionToValue.FindPtr(name)) {
            if (auto* value = versionToValuePtr->FindPtr(version)) {
                return value;
            }
        }
        return nullptr;
    }

    template <typename T>
    void Merge(THashMap<TString, T>& to, THashMap<TString, T>&& from) {
        for (auto& [key, value] : from) {
            to.emplace(key, std::move(value));
        }
    }

    template <typename T>
    void Merge(THashMap<TString, THashMap<TString, T>>& to, THashMap<TString, THashMap<TString, T>>&& from) {
        for (auto& [key, submap] : from) {
            if (auto it = to.find(key); it != to.end()) {
                Merge(it->second, std::move(submap));
            } else {
                to.emplace(key, std::move(submap));
            }
        }
    }

} // namespace

namespace NOptimizedModel {

    TVariablesInput TVariablesInput::LoadFromYson(const NYT::TNode& ysonInput) {
        TVariablesInput result;

        for (const auto& [headerField, headerValue] : ysonInput["Headers"].AsMap()) {
            result.Headers[headerField] = headerValue.AsString();
        }

        for (const auto& [inputName, inputValues] : ysonInput["ExternalInputs"].AsMap()) {
            for (const NYT::TNode& inputValue : inputValues.AsList()) {
                result.ExternalInputs[inputName].emplace_back(inputValue.AsDouble());
            }
        }

        return result;
    }

    NYT::TNode TVariablesInput::DumpToYson() const {
        NYT::TNode result;

        result["Headers"] = NYT::TNode::TMapType(Headers.begin(), Headers.end());
        result["ExternalInputs"] = NYT::TNode();
        for (const auto& [inputName, inputValues] : ExternalInputs) {
            result["ExternalInputs"][inputName] = NYT::TNode::CreateList(
                NYT::TNode::TListType(inputValues.begin(), inputValues.end()));
        }

        return result;
    }

    bool TApplyResult::HasPredict(TStringBuf name) const {
        return Predicts.contains(name);
    }

    float TApplyResult::GetPredict(TStringBuf name) const {
        const float* result = Predicts.FindPtr(name);
        if (!result) {
            return 0.5;
        }
        return *result;
    }

    void TApplyResult::AddPredict(TStringBuf name, float value) {
        Predicts.emplace(name, value);
    }

    const THashMap<TString, float>& TApplyResult::GetAllPredicts() const {
        return Predicts;
    }

    bool TApplyResult::HasEmbedding(TStringBuf name) const {
        return Embeddings.contains(name);
    }

    const TEmbedding* TApplyResult::GetEmbedding(TStringBuf name) const {
        return Embeddings.FindPtr(name);
    }

    void TApplyResult::AddEmbedding(TStringBuf name, const TEmbedding& embedding) {
        Embeddings.emplace(name, embedding);
    }

    const THashMap<TString, TEmbedding>& TApplyResult::GetAllEmbeddings() const {
        return Embeddings;
    }

    float TApplyResult::GetParameter(TStringBuf name) const {
        return Parameters.at(name);
    }

    const THashMap<TString, float>& TApplyResult::GetAllParameters() const {
        return Parameters;
    }

    const TString& TApplyResult::GetFlagsVersion() const {
        return FlagsVersion;
    }

    const THashMap<TString, TString>& TApplyResult::GetSubmodelsForExport() const {
        return SubmodelsForExport;
    }

    const TSet<NFactorSlices::TFullFactorIndex>& TApplyResult::GetClampedFactors() const {
        return ClampedFactors;
    }

    TVector<NYT::TNode> TApplyResult::DumpToYson() const {
        TVector<NYT::TNode> result;

        NYT::TNode templ;
        templ["FlagsVersion"] = FlagsVersion;
        {
            auto& dst = result.emplace_back(templ);
            dst["Type"] = "SubmodelsForExport";
            dst["Name"] = NYT::TNode::CreateEntity();
            dst["Data"] = NYT::TNode::TMapType(SubmodelsForExport.begin(), SubmodelsForExport.end());
        }

        {
            auto& dst = result.emplace_back(templ);
            dst["Type"] = "Predicts";
            dst["Name"] = NYT::TNode::CreateEntity();
            dst["Data"] = NYT::TNode::TMapType(Predicts.begin(), Predicts.end());
        }

        {
            auto& dst = result.emplace_back(templ);
            dst["Type"] = "Parameters";
            dst["Name"] = NYT::TNode::CreateEntity();
            dst["Data"] = NYT::TNode::TMapType(Parameters.begin(), Parameters.end());
        }


        templ["Type"] = "Embeds";
        for (const auto& [embeddingName, embeddingInfo] : Embeddings) {
            auto& dst = result.emplace_back(templ);
            dst["Name"] = embeddingName;
            dst["Version"] = embeddingInfo.Version;
            dst["Data"] = NYT::TNode::CreateList(
                NYT::TNode::TListType(embeddingInfo.Data.begin(), embeddingInfo.Data.end()));
        }

        return result;
    }

    float TApplyExResult::GetPredict(TStringBuf predictName, TStringBuf version) const {
        const float* result = FindPtrFromMapWithNameAndVersion(Predicts, predictName, version);
        if (result) {
            return *result;
        }
        return 0.5;
    }

    const TEmbedding* TApplyExResult::GetEmbedding(TStringBuf embeddingName, TStringBuf version) const {
        return FindPtrFromMapWithNameAndVersion(Embeddings, embeddingName, version);
    }

    float TApplyExResult::GetParameter(TStringBuf parameterName, TStringBuf version) const {
        return Parameters.at(parameterName).at(version);
    }

    float TApplyExResult::GetPredict(TStringBuf predictName) const {
        if (!PredictToDefaultVersion.contains(predictName)) {
            return 0.5;
        }
        return GetPredict(predictName, PredictToDefaultVersion.at(predictName));
    }

    const TEmbedding* TApplyExResult::GetEmbedding(TStringBuf embeddingName) const {
        if (!EmbeddingToDefaultVersion.contains(embeddingName)) {
            return nullptr;
        }
        return GetEmbedding(embeddingName, EmbeddingToDefaultVersion.at(embeddingName));
    }

    const THashMap<TString, THashMap<TString, TEmbedding>>& TApplyExResult::GetAllEmbeddings() const {
        return Embeddings;
    }

    float TApplyExResult::GetParameter(TStringBuf parameterName) const {
        return GetParameter(parameterName, ParameterToDefaultVersion.at(parameterName));
    }

    const TString& TApplyExResult::GetFlagsVersion() const {
        return FlagsVersion;
    }

    const THashMap<TString, TString>& TApplyExResult::GetSubmodelsForExport() const {
        return SubmodelsForExport;
    }

    void TApplyExResult::Merge(TApplyExResult&& other) {
        ::Merge(Predicts, std::move(other.Predicts));
        ::Merge(Embeddings, std::move(other.Embeddings));
        ::Merge(Parameters, std::move(other.Parameters));

        ::Merge(PredictToDefaultVersion, std::move(other.PredictToDefaultVersion));
        ::Merge(EmbeddingToDefaultVersion, std::move(other.EmbeddingToDefaultVersion));
        ::Merge(ParameterToDefaultVersion, std::move(other.ParameterToDefaultVersion));

        ::Merge(SubmodelsForExport, std::move(other.SubmodelsForExport));
        ClampedFactors.merge(std::move(other.ClampedFactors));
    }

    // static
    NProto::TSerializedApplyExResult TApplyExResult::SerializeToProto(const TApplyExResult& arg) {
        NProto::TSerializedApplyExResult result;

        result.SetFlagsVersion(arg.FlagsVersion);
        for (const auto& [name, version] : arg.SubmodelsForExport) {
            *result.AddSubmodelsForExport() = MakeNameToVersion(name, version);
        }

        for (const auto& [name, version] : arg.PredictToDefaultVersion) {
            *result.AddPredictToDefaultVersion() = MakeNameToVersion(name, version);
        }
        for (const auto& [name, version] : arg.EmbeddingToDefaultVersion) {
            *result.AddEmbeddingToDefaultVersion() = MakeNameToVersion(name, version);
        }
        for (const auto& [name, version] : arg.ParameterToDefaultVersion) {
            *result.AddParameterToDefaultVersion() = MakeNameToVersion(name, version);
        }

        for (const auto& [name, versionToPredict] : arg.Predicts) {
            for (const auto& [version, predict] : versionToPredict) {
                NProto::TSingleValueStorage protoPredict;
                protoPredict.SetName(name);
                protoPredict.SetVersion(version);
                protoPredict.SetValue(predict);
                *result.AddPredicts() = std::move(protoPredict);
            }
        }

        for (const auto& [name, versionToEmbedding] : arg.Embeddings) {
            for (const auto& [version, embedding] : versionToEmbedding) {
                Y_ENSURE(version, embedding.Version);
                NEmbeddingsTransfer::NProto::TModelEmbedding modelEmbedding;
                SetEmbeddingData(modelEmbedding, name, embedding);
                *result.AddEmbeddings() = std::move(modelEmbedding);
            }
        }

        for (const auto& [name, versionToParameter] : arg.Parameters) {
            for (const auto& [version, parameter] : versionToParameter) {
                NProto::TSingleValueStorage protoParameter;
                protoParameter.SetName(name);
                protoParameter.SetVersion(version);
                protoParameter.SetValue(parameter);
                *result.AddParameters() = std::move(protoParameter);
            }
        }
        return result;
    }

    // static
    TApplyExResult TApplyExResult::DeserializeFromProto(const NProto::TSerializedApplyExResult& arg) {
        TApplyExResult result;
        result.FlagsVersion = arg.GetFlagsVersion();

        for (const NProto::TNameToVersion& submodelForExport : arg.GetSubmodelsForExport()) {
            const TString& name = submodelForExport.GetName();
            const TString& version = submodelForExport.GetVersion();
            result.SubmodelsForExport.emplace(name, version);
        }

        for (const NProto::TNameToVersion& predictToDefaultVersion : arg.GetPredictToDefaultVersion()) {
            const TString& name = predictToDefaultVersion.GetName();
            const TString& version = predictToDefaultVersion.GetVersion();
            result.PredictToDefaultVersion.emplace(name, version);
        }
        for (const NProto::TNameToVersion& embeddingToDefaultVersion : arg.GetEmbeddingToDefaultVersion()) {
            const TString& name = embeddingToDefaultVersion.GetName();
            const TString& version = embeddingToDefaultVersion.GetVersion();
            result.EmbeddingToDefaultVersion.emplace(name, version);
        }
        for (const NProto::TNameToVersion& parameterToDefaultVersion : arg.GetParameterToDefaultVersion()) {
            const TString& name = parameterToDefaultVersion.GetName();
            const TString& version = parameterToDefaultVersion.GetVersion();
            result.ParameterToDefaultVersion.emplace(name, version);
        }

        for (const NProto::TSingleValueStorage& predict : arg.GetPredicts()) {
            const TString& name = predict.GetName();
            const TString& version = predict.GetVersion();
            const float value = predict.GetValue();
            result.Predicts[name].emplace(version, value);
        }

        for (const NEmbeddingsTransfer::NProto::TModelEmbedding& modelEmbedding : arg.GetEmbeddings()) {
            const TString& name = modelEmbedding.GetName();
            const TString& version = modelEmbedding.GetVersion();

            TVector<float> decompressedDAta =
                NEmbeddingsTransfer::DecompressEmbedding(
                    modelEmbedding.GetCompressedData(),
                    modelEmbedding.GetCompressionType());
            TEmbedding embedding(version, std::move(decompressedDAta));
            result.Embeddings[name].emplace(version, std::move(embedding));
        }

        for (const NProto::TSingleValueStorage& parameter : arg.GetParameters()) {
            const TString& name = parameter.GetName();
            const TString& version = parameter.GetVersion();
            const float value = parameter.GetValue();
            result.Parameters[name].emplace(version, value);
        }
        return result;
    }

    NYT::TNode TApplyExResult::DumpToYson() const {
        return ProtoToYtNode(SerializeToProto(*this));
    }

    NNeuralNetApplier::TMatrix* GetInputMatrixPtr(NNeuralNetApplier::TEvalContext* evalContext, const TString& inputName, ui64 rows, ui64 columns) {
        auto res = dynamic_cast<NNeuralNetApplier::TMatrix*>((*evalContext)[inputName].Get());
        if (res) {
            res->Resize(rows, columns);
            res->FillZero();
        } else {
            auto newMatrix = MakeIntrusive<NNeuralNetApplier::TMatrix>(rows, columns);
            res = newMatrix.Get();
            (*evalContext)[inputName] = newMatrix;
        }
        return res;
    }

    void TOptimizedModelApplier::Init(const NNeuralNetApplier::TModel& model, const NBuildModels::NProto::EApplyMode applyMode, const THashSet<TString>& submodels) {
        NYT::TNode metaData;
        try {
            metaData = NYT::NodeFromYsonString(model.GetMetaData());
        } catch (...) {
            SEARCH_ERROR << "Failed to initialize optimized_model.dssm: bad format of meta data" << Endl;
            throw;
        }

        Y_ENSURE(metaData.HasKey("FormatVersion"), "Unknown format version of model meta data.");
        TVector<TString> modelFormatVersion = StringSplitter(metaData["FormatVersion"].AsString()).Split('.');
        TVector<TString> clientFormatVersion = StringSplitter(FORMAT_VERSION).Split('.');
        Y_ENSURE(modelFormatVersion.size() == 3, "Wrong format of model meta data format version.");
        Y_ENSURE((FromString<ui64>(modelFormatVersion[0]) == FromString<ui64>(clientFormatVersion[0])) &&
            (FromString<ui64>(modelFormatVersion[1]) <= FromString<ui64>(clientFormatVersion[1])),
            "Model is incompatible with this version of optimized_model.dssm applier");

        NBuildModels::NProto::TModelsBundleConfig bundleConfig;
        YtNodeToProto(metaData["BundleConfig"], bundleConfig);
        CheckBundleConfig(bundleConfig);
        bundleConfig = FilterBundleConfigForApplyMode(bundleConfig, applyMode, submodels);

        for (const NBuildModels::NProto::TFlagConfig& flagsConfig : bundleConfig.GetFlagsConfigs()) {
            FlagsModels[flagsConfig.GetId()] = GetModelWithInfo(model, bundleConfig, flagsConfig, false);
            if (flagsConfig.GetDefault()) {
                DefaultFlagsVersion = flagsConfig.GetId();
                FullOptimizedModel = GetModelWithInfo(model, bundleConfig, flagsConfig, true);
            }
        }
    }

    TVector<TSubmodelOutputInfo> TOptimizedModelApplier::GetAllSubmodels(const NNeuralNetApplier::TModel& model, const NBuildModels::NProto::EApplyMode applyMode) {
        NYT::TNode metaData = NYT::NodeFromYsonString(model.GetMetaData());
        NBuildModels::NProto::TModelsBundleConfig bundleConfig;
        YtNodeToProto(metaData["BundleConfig"], bundleConfig);
        // return list in the same order as in config, but ensure no duplicates
        // (with this, models sharing blocks tend to be grouped together)
        THashMap<TString, size_t> seenSubmodels;
        TVector<TSubmodelOutputInfo> submodels;
        for (const NBuildModels::NProto::TSubmodelInfo& submodelInfo : bundleConfig.GetSubmodelsInfos()) {
            if (submodelInfo.GetApplyConfiguration() == ToString(NBuildModels::NProto::EApplyMode::ALWAYS) ||
                submodelInfo.GetApplyConfiguration() == ToString(applyMode))
            {
                auto insertResult = seenSubmodels.emplace(submodelInfo.GetName(), submodels.size());
                if (insertResult.second) {
                    submodels.push_back(TSubmodelOutputInfo{});
                    submodels.back().ModelName = submodelInfo.GetName();
                }
                TSubmodelOutputInfo& out = submodels[insertResult.first->second];
                for (const TString& predictName : submodelInfo.GetOutputValues()) {
                    out.PredictNames.insert(predictName);
                }
                for (const NBuildModels::NProto::TLayerInfo& layer : submodelInfo.GetOutputLayers()) {
                    out.EmbeddingNames.insert(layer.GetName());
                }
            }
        }
        return submodels;
    }

    TVector<TApplyResult> TOptimizedModelApplier::ApplyBatched(
        const TApplyConfig& applyConfig,
        TConstArrayRef<TVariablesInput> inputs) const
    {
        Y_ENSURE(!applyConfig.ApplyAll);
        TVector<TApplyExResult> applyExResults = ApplyExBatched(applyConfig, inputs);
        TVector<TApplyResult> results(inputs.size());

        for (const ui32 i : xrange(inputs.size())){
            results[i].FlagsVersion = std::move(applyExResults[i].FlagsVersion);
            results[i].SubmodelsForExport = std::move(applyExResults[i].SubmodelsForExport);

            results[i].ClampedFactors = std::move(applyExResults[i].ClampedFactors);

            for (const auto& [name, _] : applyExResults[i].Predicts) {
                results[i].Predicts.emplace(name, applyExResults[i].GetPredict(name));
            }

            for (auto& [name, versionToEmbedding] : applyExResults[i].Embeddings) {
                const TString& defaultVersion = applyExResults[i].EmbeddingToDefaultVersion.at(name);
                results[i].Embeddings.emplace(name, std::move(versionToEmbedding[defaultVersion]));
            }

            for (const auto& [name, _] : applyExResults[i].Parameters) {
                results[i].Parameters.emplace(name, applyExResults[i].GetParameter(name));
            }
        }
        return results;
    }

    TVector<TApplyExResult> TOptimizedModelApplier::ApplyExBatched(
        const TApplyConfig& applyConfig,
        TConstArrayRef<TVariablesInput> inputs) const
    {
        ui32 batchSize = inputs.size();
        TVector<TApplyExResult> results(batchSize);
        auto [flagsVersion, submodelsForExport, modelWithInfoPtr] = ConfigureApplyOptions(applyConfig);

        for (TApplyExResult& result : results) {
            result.FlagsVersion = flagsVersion;
            result.SubmodelsForExport = submodelsForExport;
        }

        // for case when corresponding flags config is empty
        if (modelWithInfoPtr->SubmodelsInfos.empty()) {
            return results;
        }

        NNeuralNetApplier::TEvalContext* evalContext =
            FillEvalContextFromInputs(*modelWithInfoPtr, inputs, (applyConfig.ApplyAll ? Nothing() : MakeMaybe(submodelsForExport)), results);

        modelWithInfoPtr->Model->Apply(*evalContext);

        for (const auto& [nameAndVersion, submodelInfo] : modelWithInfoPtr->SubmodelsInfos) {
            const auto& [name, version] = nameAndVersion;

            bool isSubmodelForExport = false;
            if (auto* versionPtr = submodelsForExport.FindPtr(name)) {
                isSubmodelForExport = (*versionPtr == version);
            }

            if (!applyConfig.ApplyAll && !isSubmodelForExport) {
                continue;
            }

            for (const TString& predictName : submodelInfo.GetOutputValues()) {
                TString layerName = NNeuralNetApplier::GetNameWithVersion(predictName, version);
                TVector<TVector<float>> predicts = GetVariable(*evalContext, layerName, batchSize);

                for (const ui32 i : xrange(batchSize)) {
                    Y_ENSURE(predicts[i].size() == 1, "Expected single float value");
                    results[i].Predicts[predictName].emplace(version, predicts[i][0]);
                    if (isSubmodelForExport) {
                        results[i].PredictToDefaultVersion.emplace(predictName, version);
                    }
                }
            }

            for (const NBuildModels::NProto::TLayerInfo& layer : submodelInfo.GetOutputLayers()) {
                const TString& embeddingName = layer.GetName();
                const TString layerName = NNeuralNetApplier::GetNameWithVersion(embeddingName, version);
                TVector<TVector<float>> embeddings = GetVariable(*evalContext, layerName, batchSize);

                for (const ui32 i : xrange(batchSize)) {
                    TEmbedding embedding(version, embeddings[i]);
                    if (Y_UNLIKELY(embedding.Data.size() != layer.GetExpectedSize())) {
                        SEARCH_ERROR << "Bad value of output variable " << layerName << "; embedding.Data.size() == " << embedding.Data.size() << " != layer.GetExpectedSize(): " << layer.GetExpectedSize() << Endl;
                        continue;
                    }

                    results[i].Embeddings[embeddingName].emplace(version, std::move(embedding));
                    if (isSubmodelForExport) {
                        results[i].EmbeddingToDefaultVersion.emplace(embeddingName, version);
                    }
                }
            }

            for (const NBuildModels::NProto::TParamInfo& parameter : submodelInfo.GetOutputParameters()) {
                const TString& parameterName = parameter.GetName();
                const float value = parameter.GetValue();

                for (const ui32 i : xrange(batchSize)) {
                    results[i].Parameters[parameterName].emplace(version, value);
                    if (isSubmodelForExport) {
                        results[i].ParameterToDefaultVersion.emplace(parameterName, version);
                    }
                }
            }
        }

        return results;
    }

    TApplyResult TOptimizedModelApplier::Apply(
        const TApplyConfig& applyConfig,
        const TVariablesInput& input) const
    {
        return std::move(ApplyBatched(applyConfig, {input})[0]);
    }

    TApplyExResult TOptimizedModelApplier::ApplyEx(
        const TApplyConfig& applyConfig,
        const TVariablesInput& input) const
    {
        return std::move(ApplyExBatched(applyConfig, {input})[0]);
    }

    THashMap<TString, TString> TOptimizedModelApplier::GetSubmodelsForExport(
        const TApplyConfig& applyConfig) const
    {
        auto [flagsVersion, submodelsForExport, modelWithInfoPtr] = ConfigureApplyOptions(applyConfig);
        return submodelsForExport;
    }

    NNeuralNetApplier::TEvalContext* TOptimizedModelApplier::FillEvalContextFromInputs(
        const TModelWithInfo& modelWithInfo,
        TConstArrayRef<TVariablesInput> inputs,
        const TMaybe<THashMap<TString, TString>>& submodelsForExport,
        TVector<TApplyExResult>& applyResults) const
    {
        ui32 batchSize = inputs.size();
        NNeuralNetApplier::TEvalContext* evalContext = ThreadLocalEvalContext_.Get(nullptr, modelWithInfo.Model->AllVariablesCount());
        modelWithInfo.Model->FillEvalContextFromSamples(MakeSamplesPtrs(inputs), *evalContext);

        for (const auto& [modelNameWithVersion, submodelInfo] : modelWithInfo.SubmodelsInfos) {
            const auto& [modelName, version] = modelNameWithVersion;

            bool isSubmodelForExport = true;
            if (submodelsForExport.Defined()) {
                const TString* versionForExport = submodelsForExport->FindPtr(modelName);
                isSubmodelForExport = versionForExport && (*versionForExport == version);
            }

            for (const NBuildModels::NProto::TLayerInfo& externalInputInfo : submodelInfo.GetExternalLayers()) {
                if (!externalInputInfo.GetInputFeatures().empty()) {
                    continue;
                }

                TString inputName = NNeuralNetApplier::GetNameWithVersion(externalInputInfo.GetName(), version);
                ui64 expectedSize = externalInputInfo.GetExpectedSize();
                auto inputMatrixPtr = GetInputMatrixPtr(evalContext, inputName, batchSize, expectedSize);
                if (isSubmodelForExport) {
                    for (const ui32 i : xrange(batchSize)) {
                        SetExternalInput((*inputMatrixPtr)[i], inputs[i].ExternalInputs, inputName, expectedSize, externalInputInfo.GetNormalize());
                    }
                }
            }
        }

        for (const auto& [inputName, layerInfo] : modelWithInfo.CommonExternalLayers) {
            auto inputMatrixPtr = GetInputMatrixPtr(evalContext, inputName, batchSize, layerInfo.GetExpectedSize());

            for (const ui32 i : xrange(batchSize)) {
                if (!layerInfo.GetInputFeatures().empty() && inputs[i].InputFeatures.Defined()) {
                    NNeuralNet::SelectFactorsFromStorage(
                        inputs[i].InputFeatures.GetRef(),
                        layerInfo.GetInputFeatures(),
                        TArrayRef((*inputMatrixPtr)[i], inputMatrixPtr->GetNumColumns()),
                        &applyResults[i].ClampedFactors
                    );
                }
            }
        }

        return evalContext;
    }

    NBuildModels::NProto::TModelsBundleConfig TOptimizedModelApplier::FilterBundleConfigForApplyMode(
        const NBuildModels::NProto::TModelsBundleConfig& bundleConfig,
        const NBuildModels::NProto::EApplyMode applyMode,
        const THashSet<TString>& submodels)
    {
        NBuildModels::NProto::TModelsBundleConfig res;
        for (const NBuildModels::NProto::TSubmodelInfo& submodelInfo : bundleConfig.GetSubmodelsInfos()) {
            if ((submodelInfo.GetApplyConfiguration() == ToString(NBuildModels::NProto::EApplyMode::ALWAYS) ||
                submodelInfo.GetApplyConfiguration() == ToString(applyMode)) &&
                (submodels.empty() || submodels.contains(submodelInfo.GetName())))
            {
                (*res.AddSubmodelsInfos()) = submodelInfo;
            }
        }
        (*res.MutableFlagsConfigs()) = bundleConfig.GetFlagsConfigs();
        return res;
    }

    TModelWithInfo TOptimizedModelApplier::GetModelWithInfo(
        const NNeuralNetApplier::TModel& model,
        const NBuildModels::NProto::TModelsBundleConfig& bundleConfig,
        const NBuildModels::NProto::TFlagConfig& flagsConfig,
        const bool needFullModel)
    {
        TModelWithInfo res;
        TSet<TString> externalInputs;
        TSet<TString> requiredOutputs;
        for (const NBuildModels::NProto::TSubmodelInfo& submodelInfo : bundleConfig.GetSubmodelsInfos()) {
            TString name = submodelInfo.GetName();
            TString version = submodelInfo.GetVersion();

            auto collectSubmodelInfo = [&]() {
                res.SubmodelsInfos[std::make_pair(name, version)] = submodelInfo;

                for (const NBuildModels::NProto::TLayerInfo& layer : submodelInfo.GetExternalLayers()) {
                    if (layer.GetInputFeatures().empty()) {
                        externalInputs.emplace(NNeuralNetApplier::GetNameWithVersion(layer.GetName(), version));
                    } else {
                        externalInputs.emplace(layer.GetName());
                        res.CommonExternalLayers[layer.GetName()] = layer;
                    }
                }

                for (const TString& predictName : submodelInfo.GetOutputValues()) {
                    requiredOutputs.emplace(NNeuralNetApplier::GetNameWithVersion(predictName, version));
                }

                for (const NBuildModels::NProto::TLayerInfo& layer : submodelInfo.GetOutputLayers()) {
                    requiredOutputs.emplace(NNeuralNetApplier::GetNameWithVersion(layer.GetName(), version));
                }
            };

            if (needFullModel) {
                collectSubmodelInfo();
            }

            for (const NBuildModels::NProto::TSubmodelInfo& neededModelInfo : flagsConfig.GetUsedModels()) {
                if ((name == neededModelInfo.GetName()) && (version == neededModelInfo.GetVersion())) {
                    res.SubmodelsForExport[name] = version;
                    collectSubmodelInfo();
                }
            }
        }

        res.Model = model.GetSubmodel(requiredOutputs, externalInputs);
        return res;
    }

    TOptimizedModelApplier::TConfigureApplyOptionsResult TOptimizedModelApplier::ConfigureApplyOptions(
        const TApplyConfig& applyConfig) const
    {
        TString flagsVersion = FlagsModels.contains(applyConfig.FlagsVersion) ? applyConfig.FlagsVersion : DefaultFlagsVersion;
        THashMap<TString, TString> submodelsForExport = FlagsModels.at(flagsVersion).SubmodelsForExport;
        const TModelWithInfo* modelWithInfoPtr = &FlagsModels.at(flagsVersion);

        // Apply rewriting.
        if (!applyConfig.ModelsVersionsRewrites.empty()) {
            for (const auto& [modelName, version] : applyConfig.ModelsVersionsRewrites) {
                submodelsForExport[modelName] = version;
            }
            flagsVersion = FULL_OPTIMIZED_MODEL_FLAGS_VERSION;
            modelWithInfoPtr = &FullOptimizedModel;
        }

        if (applyConfig.ApplyAll) {
            modelWithInfoPtr = &FullOptimizedModel;
        }

        return {std::move(flagsVersion), std::move(submodelsForExport), modelWithInfoPtr};
    }

    void TOptimizedModelApplier::SetExternalInput(float* destinationBeginPtr,
        const THashMap<TString, TVector<float>>& externalInputs,
        const TString& inputName, const ui64 expectedSize, bool normalize)
    {
        if (Y_UNLIKELY(!externalInputs.contains(inputName) || externalInputs.at(inputName).size() != expectedSize)) {
            // SEARCH_ERROR << "Bad value of input variable " << inputName << Endl;
            return;
        }

        TVector<float> embedding = externalInputs.at(inputName);
        if (normalize) {
            if (Y_UNLIKELY(!NNeuralNetApplier::TryNormalize(embedding))) {
                // SEARCH_ERROR << "Bad value of input variable " << inputName << Endl;
                return;
            }
        }

        std::move(embedding.begin(), embedding.end(), destinationBeginPtr);
    }

    TVector<TVector<float>> TOptimizedModelApplier::GetVariable(const NNeuralNetApplier::TEvalContext& evalContext,
        const TString& variable, const ui32 batchSize)
    {
        const NNeuralNetApplier::TMatrix* matrix = VerifyDynamicCast<NNeuralNetApplier::TMatrix*>(evalContext.at(variable).Get());
        Y_ENSURE(matrix->GetNumRows() == batchSize, "Expected " << batchSize << " rows in matrix " << variable
            << ", but got " << matrix->GetNumRows());

        TVector<TVector<float>> result(batchSize);
        for (const ui32 i : xrange(batchSize)) {
            result[i] = TVector<float>((*matrix)[i], (*matrix)[i] + matrix->GetNumColumns());
        }
        return result;
    }

} // NOptimizedModel
