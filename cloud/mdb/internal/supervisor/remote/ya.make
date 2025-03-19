GO_LIBRARY()

OWNER(g:mdb)

SRCS(remote.go)

GO_XTEST_SRCS(remote_test.go)

END()

RECURSE(gotest)
