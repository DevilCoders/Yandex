OWNER(g:datacloud)

PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL cloud/mdb/datacloud/private_api)
ONLY_TAGS(
    GO_PROTO
    JAVA_PROTO
)
GRPC()
SRCS(
    cloud.proto
    cloud_service.proto
    cluster.proto
    cluster_service.proto
)

PEERDIR(
    cloud/mdb/datacloud/private_api/datacloud/v1
    cloud/mdb/datacloud/private_api/datacloud/console/v1
    cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1
)

USE_COMMON_GOOGLE_APIS(
    rpc/status
)

END()
