OWNER(g:ci)
PROTO_LIBRARY(ci-proto)

GRPC()

SRCS(
    common.proto
    internal_api.proto
    frontend_flow_api.proto
    frontend_on_commit_flow_launch_api.proto
    frontend_release_api.proto
    frontend_project_api.proto
    frontend_timeline_api.proto
    security_api.proto
    storage_api.proto
)
PEERDIR(
    ci/tasklet/common/proto
    ci/proto/common
)

EXCLUDE_TAGS(GO_PROTO)

END()

RECURSE(
    admin
    autocheck
    ayamler
    common
    event
    observer
    storage
    pciexpress
)
