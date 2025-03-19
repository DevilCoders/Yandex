OWNER(g:datacloud)

PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL cloud/mdb/datacloud/private_api)
PY_NAMESPACE(datacloud.v1)
ONLY_TAGS(
    GO_PROTO
    JAVA_PROTO
    PY3_PROTO
)
GRPC()
SRCS(
    cluster.proto
    operation.proto
    paging.proto
)

USE_COMMON_GOOGLE_APIS(
    rpc/status
)

END()
