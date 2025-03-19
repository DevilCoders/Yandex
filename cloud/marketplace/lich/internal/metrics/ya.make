GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    grpc_stats.go
    http_stats.go
    hub.go
)

GO_TEST_SRCS(
    http_stats_test.go
)

END()

RECURSE(
    gotest
)