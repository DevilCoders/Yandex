PROTO_LIBRARY()

OWNER(g:tasklet)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    schema.proto
)

PEERDIR(
    tasklet/api
    tasklet/services/ci/proto
)

END()
