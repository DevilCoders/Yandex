GO_LIBRARY()

OWNER(g:mdb)

SRCS(juggler.go)

GO_XTEST_SRCS(juggler_test.go)

END()

RECURSE(
    gotest
    http
    mocks
    push
)
