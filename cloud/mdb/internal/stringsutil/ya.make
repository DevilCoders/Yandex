GO_LIBRARY()

OWNER(g:mdb)

SRCS(stringsutil.go)

GO_XTEST_SRCS(stringsutil_test.go)

END()

RECURSE(gotest)
