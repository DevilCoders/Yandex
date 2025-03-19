GO_LIBRARY()

OWNER(g:mdb)

SRCS(string.go)

GO_XTEST_SRCS(string_test.go)

END()

RECURSE(gotest)
