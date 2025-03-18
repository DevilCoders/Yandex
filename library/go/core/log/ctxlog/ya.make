GO_LIBRARY()

OWNER(
    sidh
    g:go-library
)

SRCS(ctxlog.go)

GO_TEST_SRCS(ctxlog_test.go)

END()

RECURSE(gotest)
