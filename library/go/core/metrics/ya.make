GO_LIBRARY()

OWNER(g:go-library)

SRCS(
    buckets.go
    metrics.go
)

GO_TEST_SRCS(buckets_test.go)

END()

RECURSE(
    collect
    gotest
    internal
    mock
    nop
    prometheus
    solomon
)
