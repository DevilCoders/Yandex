GO_LIBRARY()

OWNER(g:mdb)

SRCS(locker.go)

GO_XTEST_SRCS(locker_test.go)

END()

RECURSE(gotest)
