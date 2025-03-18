OWNER(g:ci)
PROTO_LIBRARY(ci-flow-engine-test-proto)

SRCS(
    test_resources.proto
)

PEERDIR(
    ci/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()

