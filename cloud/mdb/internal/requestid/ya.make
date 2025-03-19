GO_LIBRARY()

OWNER(g:mdb)

SRCS(requestid.go)

GO_XTEST_SRCS(requestid_test.go)

END()

RECURSE(gotest)
