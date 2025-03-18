PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    create_issue.proto
)

PEERDIR(
    ci/tasklet/common/proto

    tasklet/api
    tasklet/services/ci/proto
    tasklet/services/yav/proto
)

END()
