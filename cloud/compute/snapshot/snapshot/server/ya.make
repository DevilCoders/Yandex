GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    errors.go
    metrics.go
    server.go
    start.go
    wrapper.go
)

GO_TEST_SRCS(start_test.go)

END()

RECURSE(gotest)
