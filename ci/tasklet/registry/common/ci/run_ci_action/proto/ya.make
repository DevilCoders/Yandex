PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    run_ci_action.proto
)

PEERDIR(
    ci/tasklet/common/proto

    tasklet/api
    tasklet/services/ci/proto
)

END()
