PROTO_LIBRARY()

OWNER(g:tasklet)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    sawmill.proto
)

PEERDIR(
    ci/tasklet/registry/demov2/woodflow/sawmill/proto

    tasklet/api
    tasklet/services/ci/proto
)

END()
