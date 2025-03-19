GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    postgresql.go
    tests.go
)

GO_TEST_SRCS(postgresql_test.go)

END()

RECURSE(gotest)
