#include "knn.h"

namespace NFactKnn {
    constexpr size_t KNN_SEARCH_SIZE = 100;
    const TString KNN_MODEL_FINGER_PRINT = "fp";

    void InitKnnModel(const TString& dataPath, bool needPrecharge, NKnnOps::TKnnIndexHolder::TRef& knnModel) {
        NKnnOps::TKnnIndexConfig config;
        config.OpenMode = needPrecharge ? NKnnOps::EKnnIndexFilesOpenMode::OnlyLockedMemory : NKnnOps::EKnnIndexFilesOpenMode::JustFileMap;
        knnModel.Reset(NKnnOps::TKnnIndexHolder::Create(config, dataPath));
    }

    TVector<NKnnOps::TKnnSearchResult> FindNearestQueries(const NKnnOps::TKnnIndexHolder::TRef& factQueriesKnnModel, size_t size, const TVector<float>& embed) {
        NKnnOps::TKnnSearchArgs knnArgs;
        knnArgs.TopSize = size;
        knnArgs.ActiveNodesSize = KNN_SEARCH_SIZE;
        knnArgs.EmbedModelName = KNN_MODEL_FINGER_PRINT;
        knnArgs.QueryEmbed = embed;
        return factQueriesKnnModel->FindNearest(knnArgs);
    }
}
