GO_LIBRARY()

OWNER(g:mdb)

SRCS(memoized.go)

GO_XTEST_SRCS(memoized_test.go)

END()

RECURSE(gotest)
