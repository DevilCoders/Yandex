PROTO_LIBRARY()

OWNER(g:tasklet)

TASKLET()

SRCS(
    update_bio_beta.proto
)

PEERDIR(
    tasklet/api
    tasklet/services/yav/proto
    ci/tasklet/common/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
