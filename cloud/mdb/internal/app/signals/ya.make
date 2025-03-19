GO_LIBRARY()

OWNER(g:mdb)

SRCS(wait.go)

GO_TEST_SRCS(wait_test.go)

END()

RECURSE(gotest)
