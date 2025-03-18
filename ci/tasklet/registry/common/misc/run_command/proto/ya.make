PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

SRCS(
    run_command.proto
)

PEERDIR(
    ci/tasklet/common/proto

    tasklet/api
    tasklet/services/ci/proto
    tasklet/services/yav/proto
)

END()
