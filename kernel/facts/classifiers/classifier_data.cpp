#include "classifier_data.h"

namespace NFactClassifiers {
    constexpr int DEFAULT_TOP_KNN_CANDIDATES = 1;

    TClassifierData::TClassifierData(const NUnstructuredFeatures::TConfig& config)
        : Threshold(config.GetGlobalParameter("threshold"))
        , TopKNNCandidates(static_cast<size_t>(config.GetGlobalParameter("top_knn_candidates", DEFAULT_TOP_KNN_CANDIDATES)))
        , FeaturesData(config)
    {
        TIFStream classifierStream(config.GetFilePath("classifier"));
        ClassifierModel.Reset(new NMatrixnet::TMnSseDynamic());
        ClassifierModel->Load(&classifierStream);
        Y_ENSURE(ClassifierModel->GetNumFeats() <= static_cast<int>(NUnstructuredFeatures::N_FACTOR_COUNT));
    }
}

