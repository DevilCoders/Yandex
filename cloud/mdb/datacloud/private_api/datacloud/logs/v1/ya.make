OWNER(g:datacloud)

PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL cloud/mdb/datacloud/private_api)
ONLY_TAGS(
    GO_PROTO
    JAVA_PROTO
)
GRPC()
SRCS(
    log.proto
    log_service.proto
    log_source.proto
)

PEERDIR(
    cloud/mdb/datacloud/private_api/datacloud/v1
)

USE_COMMON_GOOGLE_APIS(
    rpc/status
)

END()
