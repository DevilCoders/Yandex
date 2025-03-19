GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    sqlserver.go
    test.go
)

GO_TEST_SRCS(sqlserver_test.go)

END()

RECURSE(gotest)
