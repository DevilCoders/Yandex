GO_LIBRARY()

OWNER(g:mdb)

SRCS(pgerrors.go)

GO_XTEST_SRCS(pgerrors_test.go)

END()

RECURSE(gotest)
