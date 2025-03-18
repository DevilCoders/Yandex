#pragma once

#include "index_data.h"

#include <library/cpp/hnsw/index_builder/index_writer.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/stream/file.h>

namespace NClusteredHnsw {
    template <class TItemStorageSerializer, class TItemStorage, class TItem = typename TItemStorage::TItem>
    void WriteIndex(const TClusteredHnswIndexData<TItemStorage, TItem>& data, IOutputStream& out) {
        TBufferOutput hnswIndexBuffer;
        NHnsw::WriteIndex(data.ClustersHnswIndexData, hnswIndexBuffer);

        Save(&out, static_cast<ui64>(hnswIndexBuffer.Buffer().Size()));
        out.Write(hnswIndexBuffer.Buffer().Data(), hnswIndexBuffer.Buffer().Size());

        TBufferOutput clustersBuffer;
        TItemStorageSerializer::Save(&clustersBuffer, data.ClustersItemStorage);

        Save(&out, static_cast<ui64>(clustersBuffer.Buffer().Size()));
        out.Write(clustersBuffer.Buffer().Data(), clustersBuffer.Buffer().Size());

        Save(&out, static_cast<ui32>(data.ClusterIds.size()));
        TVector<ui32> offsets(data.ClusterIds.size() + 1);
        for (size_t i = 1; i <= data.ClusterIds.size(); ++i) {
            offsets[i] = static_cast<ui32>(data.ClusterIds[i - 1].size()) + offsets[i - 1];
        }

        out.Write(offsets.data(), offsets.size() * sizeof(offsets[0]));

        for (size_t i = 0; i < data.ClusterIds.size(); ++i) {
            out.Write(data.ClusterIds[i].data(), data.ClusterIds[i].size() * sizeof(data.ClusterIds[i][0]));
        }
    }

}
