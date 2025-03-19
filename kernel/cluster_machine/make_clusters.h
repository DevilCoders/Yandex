#pragma once

#include "processor.h"

#include <kernel/cluster_machine/proto/clustering_params.pb.h>

#include <util/generic/yexception.h>


namespace NClustering {
    TResult MakeClusters(const TVector<TEmbed>& embeddings,
        const NProto::TClusteringParams& clusteringParams);
} //namespace NClustering
