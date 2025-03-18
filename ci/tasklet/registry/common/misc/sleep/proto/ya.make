PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

SRCS(
    sleep.proto
)

PEERDIR(
    ci/tasklet/common/proto

    tasklet/api
    tasklet/services/ci/proto
)

END()
