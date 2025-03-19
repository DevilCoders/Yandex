GO_LIBRARY()

OWNER(g:mdb)

SRCS(sessions.go)

GO_TEST_SRCS(sessions_test.go)

END()

RECURSE(
    gotest
    mocks
    provider
)
