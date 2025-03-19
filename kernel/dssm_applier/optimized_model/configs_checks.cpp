#include "configs_checks.h"

#include <kernel/dssm_applier/utils/utils.h>
#include <kernel/searchlog/errorlog.h>

#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/yexception.h>

void NOptimizedModel::CheckBundleConfig(const NBuildModels::NProto::TModelsBundleConfig& config) {
    Y_ENSURE(!config.GetSubmodelsInfos().empty(), "Models bundle must contain at least one model");
    THashMap<TString, TSet<TString>> submodels;
    for (const NBuildModels::NProto::TSubmodelInfo& submodelInfo : config.GetSubmodelsInfos()) {
        Y_ENSURE(submodelInfo.HasName(), "Not found required field: Name");
        Y_ENSURE(submodelInfo.HasVersion(), "Not found required field: Version");
        TString submodelNameWithVersion = NNeuralNetApplier::GetNameWithVersion(submodelInfo.GetName(),
            submodelInfo.GetVersion());
        const TString& applyMode = submodelInfo.GetApplyConfiguration();
        Y_ENSURE(!submodels[applyMode].contains(submodelNameWithVersion), submodelNameWithVersion <<
            " is listed more than once in models bundle config for apply mode " << applyMode);
        submodels[applyMode].insert(submodelNameWithVersion);
    }

    Y_ENSURE(!config.GetFlagsConfigs().empty(), "Models bundle config must contain at least one flag config");
    TSet<TString> flagsVersions;
    bool foundDefault = false;
    for (const NBuildModels::NProto::TFlagConfig& flagConfig : config.GetFlagsConfigs()) {
        Y_ENSURE(flagConfig.HasId(), "Not found required field: Id");
        Y_ENSURE(!flagsVersions.contains(flagConfig.GetId()), "Found more than one flag configs with id " <<
            flagConfig.GetId());
        flagsVersions.insert(flagConfig.GetId());
        Y_ENSURE(flagConfig.HasDefault(), "Not found required field: Default");
        Y_ENSURE(!(flagConfig.GetDefault() && foundDefault), "Found more than one default flag configs");
        foundDefault |= flagConfig.GetDefault();

        TSet<TString> usedModels;
        THashMap<TString, TSet<TString>> outputs;
        for (const NBuildModels::NProto::TSubmodelInfo& usedModel : flagConfig.GetUsedModels()) {
            Y_ENSURE(usedModel.HasName(), "Not found required field: Name");
            Y_ENSURE(usedModel.HasVersion(), "Not found required field: Version");
            TString usedModelNameWithVersion = NNeuralNetApplier::GetNameWithVersion(
                usedModel.GetName(), usedModel.GetVersion());

            bool foundModel = false;
            for (const NBuildModels::NProto::TSubmodelInfo& submodelInfo : config.GetSubmodelsInfos()) {
                TString submodelNameWithVersion = NNeuralNetApplier::GetNameWithVersion(submodelInfo.GetName(),
                    submodelInfo.GetVersion());
                if (submodelNameWithVersion == usedModelNameWithVersion) {
                    foundModel = true;
                    const TString& applyMode = submodelInfo.GetApplyConfiguration();
                    for (const TString& outputPredict : submodelInfo.GetOutputValues()) {
                        Y_ENSURE(!outputs[applyMode].contains(outputPredict), "Found more than one model with output "
                            << outputPredict << " for apply mode " << applyMode << " in flag config");
                        outputs[applyMode].insert(outputPredict);
                    }
                    for (const NBuildModels::NProto::TLayerInfo& outputLayer : submodelInfo.GetOutputLayers()) {
                        Y_ENSURE(!outputs[applyMode].contains(outputLayer.GetName()), "Found more than one model with output " <<
                            outputLayer.GetName() << " for apply mode " << applyMode << " in flag config");
                        outputs[applyMode].insert(outputLayer.GetName());
                    }
                    for (const NBuildModels::NProto::TParamInfo& outputParam : submodelInfo.GetOutputParameters()) {
                        Y_ENSURE(!outputs[applyMode].contains(outputParam.GetName()), "Found more than one model with output " <<
                            outputParam.GetName() << " for apply mode " << applyMode << " in flag config");
                        outputs[applyMode].insert(outputParam.GetName());
                    }
                }
            }

            Y_ENSURE(foundModel, usedModelNameWithVersion << " is requested in flag config, but is absent in SubmodelsInfos");
            Y_ENSURE(!usedModels.contains(usedModel.GetName()), "Found more than one version of model " <<
                usedModel.GetName() << " in flag config");
            usedModels.insert(usedModel.GetName());
        }
    }
    Y_ENSURE(foundDefault, "Not found default flag config");

}
