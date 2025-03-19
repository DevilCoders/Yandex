OWNER(g:datacloud)

PROTO_LIBRARY()

PROTO_NAMESPACE(GLOBAL cloud/mdb/datacloud/private_api)
PY_NAMESPACE(datacloud.kafka.inner.v1)
ONLY_TAGS(
    GO_PROTO
    JAVA_PROTO
    PY3_PROTO
)

GRPC()
SRCS(
    topic_service.proto
)

PEERDIR(cloud/mdb/datacloud/private_api/datacloud/v1)

END()

