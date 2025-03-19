PROTO_LIBRARY()

OWNER(g:cloud-nbs)

INCLUDE_TAGS(GO_PROTO)

SRCS(
    client.proto
    diagnostics.proto
    filesystem.proto
    fuse.proto
    http_proxy.proto
    nfs_gateway.proto
    server.proto
    storage.proto
    vhost.proto
)

PEERDIR(
    cloud/filestore/public/api/protos
    cloud/storage/core/protos
)

END()
