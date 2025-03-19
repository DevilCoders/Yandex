PROTO_LIBRARY()

OWNER(
    gotmanov
    g:wizard
)

SRCS(
    reqbundle.proto
)

PEERDIR(
    library/cpp/langmask/proto
    kernel/hitinfo/proto
    kernel/qtree/richrequest/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
