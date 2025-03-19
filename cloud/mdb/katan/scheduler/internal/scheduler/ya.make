GO_LIBRARY()

OWNER(g:mdb)

SRCS(scheduler.go)

GO_XTEST_SRCS(scheduler_test.go)

END()

RECURSE(gotest)
