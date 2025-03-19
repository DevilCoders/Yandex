GO_LIBRARY()

OWNER(g:mdb)

SRCS(workaholic.go)

GO_XTEST_SRCS(workaholic_test.go)

END()

RECURSE(gotest)
