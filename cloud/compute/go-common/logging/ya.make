GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(logging.go)

GO_TEST_SRCS(logging_test.go)

END()

RECURSE(
    ctxlog
    go-logging
    gotest
)
