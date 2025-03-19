GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    mongodb.go
    tests.go
)

GO_TEST_SRCS(mongodb_test.go)

END()

RECURSE(gotest)
