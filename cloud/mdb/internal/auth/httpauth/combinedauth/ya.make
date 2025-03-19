GO_LIBRARY()

OWNER(g:mdb)

SRCS(auth.go)

GO_XTEST_SRCS(authservice_test.go)

END()

RECURSE(gotest)
