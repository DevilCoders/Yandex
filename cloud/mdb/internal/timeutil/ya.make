GO_LIBRARY()

OWNER(g:mdb)

SRCS(timeutil.go)

GO_TEST_SRCS(timeutil_test.go)

END()

RECURSE(gotest)
