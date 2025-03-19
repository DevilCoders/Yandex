GO_LIBRARY()

OWNER(g:mdb)

SRCS(manager.go)

GO_XTEST_SRCS(manager_test.go)

END()

RECURSE(gotest)
