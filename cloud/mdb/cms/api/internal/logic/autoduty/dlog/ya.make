GO_LIBRARY()

OWNER(g:mdb)

SRCS(log.go)

GO_XTEST_SRCS(log_test.go)

END()

RECURSE(gotest)
