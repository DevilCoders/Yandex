OWNER(g:ci)
PROTO_LIBRARY(ci-ayamler-proto)

GRPC()

SRCS(
    ayamler.proto
)

ONLY_TAGS(
    GO_PROTO
    JAVA_PROTO
    PY3_PROTO
)

END()
