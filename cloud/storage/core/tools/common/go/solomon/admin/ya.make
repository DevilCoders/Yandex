GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    alert.go
    channel.go
    cluster.go
    common.go
    dashboard.go
    graph.go
    service.go
    shard.go
)

GO_TEST_SRCS(
    alert_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
