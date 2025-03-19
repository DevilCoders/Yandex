PROTO_LIBRARY()

OWNER(g:cloud-nbs)

GRPC()

INCLUDE_TAGS(GO_PROTO)

SRCS(
    service.proto
)

PEERDIR(
    cloud/filestore/public/api/protos
)

USE_COMMON_GOOGLE_APIS(
    api/annotations
)

GO_GRPC_GATEWAY_SRCS(
    service.proto
)

END()
