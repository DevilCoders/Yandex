PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    transit_issue.proto
)

PEERDIR(
    ci/tasklet/common/proto

    tasklet/api
    tasklet/services/ci/proto
    tasklet/services/yav/proto

    ci/tasklet/registry/common/tracker/create_issue/proto
)

END()
