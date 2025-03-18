PROTO_LIBRARY()

OWNER(g:ci)

TASKLET()

SRCS(
    furniture_factory.proto
)

PEERDIR(
    ci/tasklet/registry/demov2/woodflow/furniture_factory/proto

    tasklet/api
    tasklet/services/ci/proto
)

END()
