OWNER(g:ci)
PROTO_LIBRARY(ci-core-test-proto)

SRCS(
    misc.proto
    tasklet-input.proto
    validation-tasklet-input.proto
)

PEERDIR(
    ci/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()

