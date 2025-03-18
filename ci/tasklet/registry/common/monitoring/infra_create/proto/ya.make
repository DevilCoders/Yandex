PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

EXCLUDE_TAGS(JAVA_PROTO)

SRCS(
    infra_create.proto
)

PEERDIR(
    ci/tasklet/common/proto
    tasklet/api
    tasklet/services/ci/proto
    tasklet/services/yav/proto
)

END()
