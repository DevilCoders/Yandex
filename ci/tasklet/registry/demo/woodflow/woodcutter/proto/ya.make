PROTO_LIBRARY()

OWNER(g:tasklet)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    woodcutter.proto
)

PEERDIR(
    ci/tasklet/registry/demov2/woodflow/woodcutter/proto

    tasklet/api
    tasklet/services/ci/proto
)

END()
