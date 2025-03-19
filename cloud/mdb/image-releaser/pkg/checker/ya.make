GO_LIBRARY()

OWNER(g:mdb)

SRCS(checker.go)

END()

RECURSE(
    gotest
    juggler
    mocks
)
