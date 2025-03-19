GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    bound.go
    int64.go
)

GO_TEST_SRCS(int64_test.go)

END()

RECURSE(gotest)
