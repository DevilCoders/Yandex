GO_LIBRARY()

OWNER(g:mdb)

SRCS(commander.go)

GO_TEST_SRCS(commander_test.go)

END()

RECURSE(
    gotest
    mocks
)
