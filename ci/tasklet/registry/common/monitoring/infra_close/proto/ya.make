PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

EXCLUDE_TAGS(JAVA_PROTO)

SRCS(
    infra_close.proto
)

PEERDIR(
    tasklet/api
    tasklet/services/ci/proto
    tasklet/services/yav/proto
    ci/tasklet/common/proto
    ci/tasklet/registry/common/monitoring/infra_create/proto
)

END()
