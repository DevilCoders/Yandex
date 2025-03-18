GO_LIBRARY()

OWNER(
    kositsyn-pa
    g:go-library
)

SRCS(
    background.go
    core.go
    metrics.go
    options.go
    queue.go
    stat.go
)

GO_TEST_SRCS(
    core_test.go
    options_test.go
    queue_test.go
)

END()

RECURSE(
    client
    gotest
)
