OWNER(g:ci)

PROTO_LIBRARY()

GRPC()

SRCS(
    autocheck.proto
    autocheck_api.proto
    stress_test.proto
)

PEERDIR(
    ci/proto/storage
    ci/proto
)


EXCLUDE_TAGS(GO_PROTO)

END()
