PROTO_LIBRARY(demov2-woodflow-sawmill)

OWNER(g:ci)

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    sawmill.proto
)
PEERDIR(
    ci/tasklet/registry/demov2/woodflow/common/proto
    tasklet/services/ci/proto
)

END()
