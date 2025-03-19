GO_LIBRARY()

OWNER(g:mdb)

SRCS(weekends.go)

GO_XTEST_SRCS(weekends_test.go)

END()

RECURSE(gotest)
