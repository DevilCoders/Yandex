GO_LIBRARY()

OWNER(g:mdb)

SRCS(workingtime.go)

GO_XTEST_SRCS(workingtime_test.go)

END()

RECURSE(gotest)
