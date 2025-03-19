GO_LIBRARY()

OWNER(g:mdb)

SRCS(tilde.go)

GO_XTEST_SRCS(tilde_test.go)

END()

RECURSE(gotest)
