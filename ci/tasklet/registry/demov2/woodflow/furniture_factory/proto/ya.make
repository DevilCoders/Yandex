PROTO_LIBRARY(demov2-woodflow-furniture-factory)

OWNER(g:ci)

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    furniture_factory.proto
)
PEERDIR(
    ci/tasklet/registry/demov2/woodflow/common/proto
    tasklet/services/ci/proto
)

END()
