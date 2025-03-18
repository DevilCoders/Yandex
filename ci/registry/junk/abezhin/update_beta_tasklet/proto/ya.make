PROTO_LIBRARY()

OWNER(g:tasklet)

TASKLET()

SRCS(
    update_asr_beta.proto
)

PEERDIR(
    tasklet/api
    tasklet/services/yav/proto
    ci/tasklet/common/proto
    #search/priemka/yappy/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()