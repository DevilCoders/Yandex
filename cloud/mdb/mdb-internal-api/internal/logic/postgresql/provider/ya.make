GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    databases.go
    perfdiag.go
    postgresql.go
)

GO_TEST_SRCS(perfdiag_test.go)

END()

RECURSE(
    gotest
    internal
)
