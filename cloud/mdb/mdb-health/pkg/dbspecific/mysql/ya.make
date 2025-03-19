GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    mysql.go
    tests.go
)

GO_TEST_SRCS(mysql_test.go)

END()

RECURSE(gotest)
