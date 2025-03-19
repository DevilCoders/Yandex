PROTO_LIBRARY()

OWNER(g:cloud-billing)

TASKLET()

EXCLUDE_TAGS(
    GO_PROTO
    CPP_PROTO
    EXT_PROTO
    JAVA_PROTO
)

SRCS(
    run_build.proto
)

PEERDIR(
    tasklet/api
    tasklet/services/ci/proto
    tasklet/services/yav/proto
)

END()
