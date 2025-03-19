GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    mysql.go
    perfdiag.go
)

GO_TEST_SRCS(perfdiag_test.go)

END()

RECURSE(gotest)
