OWNER(g:datacloud)

PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL cloud/mdb/datacloud/private_api)
PY_NAMESPACE(datacloud.network.v1)
ONLY_TAGS(
    GO_PROTO
    JAVA_PROTO
    PY3_PROTO
)
GRPC()
SRCS(
    network.proto
    network_service.proto
    network_connection.proto
    network_connection_service.proto
    operation_service.proto
)

PEERDIR(
    cloud/mdb/datacloud/private_api/datacloud/v1
)

USE_COMMON_GOOGLE_APIS(
    rpc/status
)

END()
