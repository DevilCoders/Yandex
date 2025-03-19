#include "test_utils.h"

#include <kernel/searchlog/errorlog.h>

#include <util/folder/dirut.h>
#include <util/memory/blob.h>

namespace NOptimizedModelTest {
    const TString INPUT_DUMP = "optimized_dssm_model_bench_input";
    const TString DSSM_MODEL = "optimized_model.dssm";

    THolder<NNeuralNetApplier::TModel> GetOptimizedModel(const TString& path) {
        if (!NFs::Exists(path)) {
            SEARCH_ERROR << "Cannot find file " << path << Endl;
            return {};
        }
        auto modelBlob = TBlob::FromFileContentSingleThreaded(DSSM_MODEL);
        SEARCH_INFO << "DSSM model size: " << modelBlob.Length() << Endl;
        auto dssmModel = MakeHolder<NNeuralNetApplier::TModel>();
        dssmModel->Load(modelBlob);
        dssmModel->Init();
        return dssmModel;
    }

    THolder<NOptimizedModel::TInputDump> LoadInputDump(const TString& path) {
        if (!NFs::Exists(path)) {
            SEARCH_ERROR << "Cannot find file " << path << Endl;
            return {};
        }
        auto inputDumpBlob = TBlob::FromFileSingleThreaded(path);
        auto inputDump = MakeHolder<NOptimizedModel::TInputDump>();
        Y_PROTOBUF_SUPPRESS_NODISCARD inputDump->ParseFromArray(inputDumpBlob.AsCharPtr(), inputDumpBlob.Length());
        return inputDump;
    }

    TTestEnvironment::TTestEnvironment() {
        OptimizedModel = GetOptimizedModel(DSSM_MODEL);
        SEARCH_INFO << "Loaded DSSM model" << Endl;
        InputDump = LoadInputDump(INPUT_DUMP);
        SEARCH_INFO << "Loaded input data, frame: " << InputDump->FramesSize() << Endl;
        if (OptimizedModel && InputDump) {
            OptimizedApplier = MakeHolder<NOptimizedModel::TOptimizedModelApplier>(*OptimizedModel, NBuildModels::NProto::EApplyMode::WEB);
            SEARCH_INFO << "Created optimized applier" << Endl;
        }
    }

    TTestEnvironment::~TTestEnvironment() {
    }
}
