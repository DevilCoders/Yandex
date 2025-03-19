#pragma once

#include <kernel/dssm_applier/optimized_model/protos/apply_result.pb.h>
#include <kernel/dssm_applier/optimized_model/protos/bundle_config.pb.h>

#include <kernel/dssm_applier/begemot/production_data.h>
#include <kernel/dssm_applier/embeddings_transfer/embeddings_transfer.h>
#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <kernel/embeddings_info/dssm_embeddings.h>
#include <kernel/factor_storage/factor_view.h>

#include <library/cpp/threading/thread_local/thread_local.h>
#include <library/cpp/yson/node/node.h>

#include <util/generic/maybe.h>


namespace NOptimizedModel {
    using NEmbeddingsTransfer::TEmbedding;

    const TString FULL_OPTIMIZED_MODEL_FLAGS_VERSION = "_FullOptimizedModelFlagsVersion";

    using TModelNameWithVersion = std::pair<TString, TString>;

    struct TApplyConfig {
        TString FlagsVersion;
        THashMap<TString, TString> ModelsVersionsRewrites;
        bool ApplyAll = false;
    };

    struct TVariablesInput {
        THashMap<TString, TString> Headers;
        THashMap<TString, TVector<float>> ExternalInputs;
        TMaybe<TMultiConstFactorView> InputFeatures;

        static TVariablesInput LoadFromYson(const NYT::TNode& ysonInput);
        NYT::TNode DumpToYson() const;
    };

    class TApplyResult {
    private:
        THashMap<TString, float> Predicts;
        THashMap<TString, TEmbedding> Embeddings;

        THashMap<TString, float> Parameters;

        TString FlagsVersion;
        THashMap<TString, TString> SubmodelsForExport;

        TSet<NFactorSlices::TFullFactorIndex> ClampedFactors;

    public:
        friend class TOptimizedModelApplier;

        bool HasPredict(TStringBuf name) const;
        void AddPredict(TStringBuf name, float value);
        float GetPredict(TStringBuf name) const;
        const THashMap<TString, float>& GetAllPredicts() const;

        bool HasEmbedding(TStringBuf name) const;
        void AddEmbedding(TStringBuf name, const TEmbedding& embedding);
        const TEmbedding* GetEmbedding(TStringBuf name) const;
        const THashMap<TString, TEmbedding>& GetAllEmbeddings() const;

        float GetParameter(TStringBuf name) const;
        const THashMap<TString, float>& GetAllParameters() const;

        const TString& GetFlagsVersion() const;
        const THashMap<TString, TString>& GetSubmodelsForExport() const;

        const TSet<NFactorSlices::TFullFactorIndex>& GetClampedFactors() const;

        TVector<NYT::TNode> DumpToYson() const;
    };

    class TApplyExResult {
    private:
        // hash_map<predictName, hash_map<version, value>>
        THashMap<TString, THashMap<TString, float>> Predicts;

        // hash_map<embeddingName, hash_map<version, value>>
        THashMap<TString, THashMap<TString, TEmbedding>> Embeddings;

        // hash_map<parameterName, hash_map<version, value>>
        THashMap<TString, THashMap<TString, float>> Parameters;

        // hash_map<predictName, defaultVersion>
        THashMap<TString, TString> PredictToDefaultVersion;

        // hash_map<embeddingName, defaultVersion>
        THashMap<TString, TString> EmbeddingToDefaultVersion;

        // hash_map<parameterName, defaultVersion>
        THashMap<TString, TString> ParameterToDefaultVersion;

        TString FlagsVersion;
        // hash_map<modelName, version>
        THashMap<TString, TString> SubmodelsForExport;

        TSet<NFactorSlices::TFullFactorIndex> ClampedFactors;

    public:
        friend class TOptimizedModelApplier;

        float GetPredict(TStringBuf predictName, TStringBuf version) const;
        const TEmbedding* GetEmbedding(TStringBuf embeddingName, TStringBuf version) const;
        float GetParameter(TStringBuf parameterName, TStringBuf version) const;

        // Version going to be extracted from TApplyConfig rules.
        float GetPredict(TStringBuf predictName) const;
        const TEmbedding* GetEmbedding(TStringBuf embeddingName) const;
        const THashMap<TString, THashMap<TString, TEmbedding>>& GetAllEmbeddings() const;
        float GetParameter(TStringBuf parameterName) const;

        const TString& GetFlagsVersion() const;
        const THashMap<TString, TString>& GetSubmodelsForExport() const;

        NYT::TNode DumpToYson() const;

        // Works like std::map::merge. FlagsVersion will not be touched.
        void Merge(TApplyExResult&& other);

        // TODO(hygge): Delete it after cleanup.
        THashMap<TString, THashMap<TString, TEmbedding>>& MutableAllEmbeddings() {
            return Embeddings;
        }
        THashMap<TString, TString>& MutableSubmodelsForExport() {
            return SubmodelsForExport;
        }
        THashMap<TString, TString>& MutableEmbeddingToDefaultVersion() {
            return EmbeddingToDefaultVersion;
        }

        static NProto::TSerializedApplyExResult SerializeToProto(const TApplyExResult& arg);
        static TApplyExResult DeserializeFromProto(const NProto::TSerializedApplyExResult& arg);
    };

    struct TModelWithInfo {
        NNeuralNetApplier::TModelPtr Model;
        THashMap<TString, NBuildModels::NProto::TLayerInfo> CommonExternalLayers;
        THashMap<TModelNameWithVersion, NBuildModels::NProto::TSubmodelInfo> SubmodelsInfos;
        THashMap<TString, TString> SubmodelsForExport;
    };

    struct TSubmodelOutputInfo {
        TString ModelName;
        THashSet<TString> PredictNames;
        THashSet<TString> EmbeddingNames;
    };

    class TOptimizedModelApplier {
    public:
        TOptimizedModelApplier(const NNeuralNetApplier::TModel& model, const NBuildModels::NProto::EApplyMode applyMode, const THashSet<TString>& submodels = {}) {
            Init(model, applyMode, submodels);
        }

        void Init(const NNeuralNetApplier::TModel& model, const NBuildModels::NProto::EApplyMode applyMode, const THashSet<TString>& submodels = {});
        TApplyResult Apply(const TApplyConfig& applyConfig, const TVariablesInput& input) const;
        TVector<TApplyResult> ApplyBatched(const TApplyConfig& applyConfig, TConstArrayRef<TVariablesInput> inputs) const;

        TApplyExResult ApplyEx(const TApplyConfig& applyConfig, const TVariablesInput& input) const;
        TVector<TApplyExResult> ApplyExBatched(const TApplyConfig& applyConfig, TConstArrayRef<TVariablesInput> inputs) const;

        THashMap<TString, TString> GetSubmodelsForExport(const TApplyConfig& applyConfig = {}) const;

        static TVector<TSubmodelOutputInfo> GetAllSubmodels(const NNeuralNetApplier::TModel& model, const NBuildModels::NProto::EApplyMode applyMode);

    private:
        struct TConfigureApplyOptionsResult {
            TString FlagsVersion;
            THashMap<TString, TString> SubmodelsForExport;
            const TModelWithInfo* ModelWithInfoPtr;
        };

    private:
        NNeuralNetApplier::TEvalContext* FillEvalContextFromInputs(
            const TModelWithInfo& modelWithInfo,
            TConstArrayRef<TVariablesInput> inputs,
            const TMaybe<THashMap<TString, TString>>& submodelsForExport,
            TVector<TApplyExResult>& applyResults) const; // applyExResults are needed here to fill info about clamped factors

        static NBuildModels::NProto::TModelsBundleConfig FilterBundleConfigForApplyMode(
            const NBuildModels::NProto::TModelsBundleConfig& bundleConfig,
            const NBuildModels::NProto::EApplyMode applyMode,
            const THashSet<TString>& submodels);

        static TModelWithInfo GetModelWithInfo(
            const NNeuralNetApplier::TModel& model,
            const NBuildModels::NProto::TModelsBundleConfig& bundleConfig,
            const NBuildModels::NProto::TFlagConfig& flagsConfig,
            const bool needFullModel = true);

        TConfigureApplyOptionsResult ConfigureApplyOptions(const TApplyConfig& applyConfig) const;

        static void SetExternalInput(float* destinationBeginPtr,
            const THashMap<TString, TVector<float>>& externalInputs,
            const TString& inputName, const ui64 expectedSize,
            bool normalize);

        static TVector<TVector<float>> GetVariable(const NNeuralNetApplier::TEvalContext& evalContext,
            const TString& variable, const ui32 batchSize);

        TModelWithInfo FullOptimizedModel;
        THashMap<TString, TModelWithInfo> FlagsModels;
        TString DefaultFlagsVersion;
        inline static NThreading::TThreadLocalValue<NNeuralNetApplier::TEvalContext> ThreadLocalEvalContext_;
    };

} // namespace NOptimizedModel
