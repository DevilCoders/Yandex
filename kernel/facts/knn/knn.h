#pragma once

#include <quality/relev_tools/knn/lib/knn_index_holder.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NFactKnn {
    const float KNN_DOT_PRODUCT_LIMIT = 0.7;

    void InitKnnModel(const TString& dataPath, bool needPrecharge, NKnnOps::TKnnIndexHolder::TRef& knnModel);

    TVector<NKnnOps::TKnnSearchResult> FindNearestQueries(const NKnnOps::TKnnIndexHolder::TRef& factQueriesKnnModel, size_t size, const TVector<float>& embed);
}
