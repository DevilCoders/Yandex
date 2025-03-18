#pragma once

#include "index_base.h"

#include <library/cpp/hnsw/index_builder/index_data.h>

#include <util/generic/vector.h>

namespace NClusteredHnsw {
    template <class TItemStorage, class TItem = typename TItemStorage::TItem>
    struct TClusteredHnswIndexData {
        TItemStorage ClustersItemStorage;
        NHnsw::THnswIndexData ClustersHnswIndexData;
        TVector<TVector<ui32>> ClusterIds;
    };

}
