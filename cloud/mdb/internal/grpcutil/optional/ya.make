GO_LIBRARY()

OWNER(g:mdb)

SRCS(optional.go)

GO_TEST_SRCS(optional_test.go)

END()

RECURSE(gotest)
