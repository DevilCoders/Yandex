GO_LIBRARY()

SRCS(
    client.go
    consts.go
    lock.go
    metrics.go
    query_registry.go
)

END()

RECURSE(
    test
)
