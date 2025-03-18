GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(
    background.go
    core.go
    options.go
    queue.go
)

GO_TEST_SRCS(
    core_test.go
    queue_test.go
)

END()

RECURSE(gotest)
