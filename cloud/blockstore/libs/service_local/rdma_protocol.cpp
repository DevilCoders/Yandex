#include "rdma_protocol.h"

#include <cloud/blockstore/libs/rdma/protobuf.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <util/generic/singleton.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

NRdma::TProtoMessageSerializer* TBlockStoreProtocol::Serializer()
{
    struct TSerializer : NRdma::TProtoMessageSerializer
    {
        TSerializer()
        {
            RegisterProto<NProto::TReadDeviceBlocksRequest>(
                ReadDeviceBlocksRequest);

            RegisterProto<NProto::TReadDeviceBlocksResponse>(
                ReadDeviceBlocksResponse);

            RegisterProto<NProto::TWriteDeviceBlocksRequest>(
                WriteDeviceBlocksRequest);

            RegisterProto<NProto::TWriteDeviceBlocksResponse>(
                WriteDeviceBlocksResponse);

            RegisterProto<NProto::TZeroDeviceBlocksRequest>(
                ZeroDeviceBlocksRequest);

            RegisterProto<NProto::TZeroDeviceBlocksResponse>(
                ZeroDeviceBlocksResponse);
        }
    };

    return Singleton<TSerializer>();
}

}   // namespace NCloud::NBlockStore::NStorage
