PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

EXCLUDE_TAGS(JAVA_PROTO)

SRCS(
    juggler_watch.proto
)

PEERDIR(
    tasklet/api
    tasklet/services/ci/proto
)

END()
