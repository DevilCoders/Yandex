GO_LIBRARY()

OWNER(g:mdb)

SRCS(logs.go)

GO_TEST_SRCS(logs_test.go)

END()

RECURSE(gotest)
