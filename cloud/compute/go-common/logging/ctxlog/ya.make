GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    conditionallog.go
    journald.go
    log.go
)

GO_TEST_SRCS(
    examples_test.go
    log_test.go
)

END()

RECURSE(gotest)
