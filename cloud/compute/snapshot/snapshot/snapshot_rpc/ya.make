PROTO_LIBRARY()

OWNER(g:compute)

INCLUDE_TAGS(GO_PROTO)

GRPC()

USE_COMMON_GOOGLE_APIS(
    api/annotations
    rpc/code
    rpc/errdetails
    rpc/status
    type/timeofday
)

SRCS(snapshot.proto)

GO_GRPC_GATEWAY_SRCS(snapshot.proto)

END()
