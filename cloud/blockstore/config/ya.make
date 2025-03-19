PROTO_LIBRARY()

OWNER(g:cloud-nbs)

INCLUDE_TAGS(GO_PROTO)

SRCS(
    certificate.proto
    client.proto
    diagnostics.proto
    discovery.proto
    disk.proto
    features.proto
    http_proxy.proto
    logbroker.proto
    notify.proto
    plugin.proto
    rdma.proto
    server.proto
    spdk.proto
    storage.proto
    storage_local.proto
    ydbstats.proto
)

PEERDIR(
    cloud/blockstore/public/api/protos
    cloud/storage/core/protos
)

END()
