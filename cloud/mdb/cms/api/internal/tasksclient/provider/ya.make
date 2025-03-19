GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    move.go
)

GO_XTEST_SRCS(move_test.go)

END()

RECURSE(
    gotest
    tests
)
