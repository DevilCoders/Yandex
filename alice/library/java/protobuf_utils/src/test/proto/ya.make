PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:paskills)

SRCS(
    test.proto
)

PEERDIR()

EXCLUDE_TAGS(GO_PROTO PY3_PROTO PY_PROTO CPP_PROTO)

END()

