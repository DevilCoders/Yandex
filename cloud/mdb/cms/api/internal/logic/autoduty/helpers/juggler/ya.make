GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    config.go
    juggler.go
)

GO_XTEST_SRCS(juggler_test.go)

END()

RECURSE(
    gotest
    mocks
)
