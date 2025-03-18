PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

EXCLUDE_TAGS(JAVA_PROTO GO_PROTO)

SRCS(
    infra_check_events.proto
)

PEERDIR(
    tasklet/api
    tasklet/services/ci/proto
    ci/tasklet/registry/common/monitoring/infra_create/proto
)

END()
