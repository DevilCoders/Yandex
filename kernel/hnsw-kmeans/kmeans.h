#pragma once

#include <kernel/hnsw-kmeans/protos/clustering.pb.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/hnsw/index_builder/build_options.h>

#include <util/generic/size_literals.h>

namespace NHnswKMeans {

    struct TParams {
        NYT::IClientPtr Client;
        NYT::TRichYPath ItemsTable;
        NYT::TRichYPath ClustersTable;

        NHnsw::THnswBuildOptions HnswOpts;
        TMaybe<ui32> Dimension = Nothing(); // Nothing for auto-detection
        ui32 NearestClusterSearchNeighborhoodSize = 64;
        bool Continue = false; // Continue kmeans iterations on existing clustering table
        bool Verbose = false; // Output energy and iteration number to stdout
    };

    void BuildClustering(ui32 clusters, ui32 iters, TParams params, NYT::TMapReduceOperationSpec spec = {});

    void AssignClusters(TParams params, NYT::TMapOperationSpec spec = {});

    std::tuple<TBuffer, TBuffer, TBuffer> BuildHnswIndex(TParams params);
}
