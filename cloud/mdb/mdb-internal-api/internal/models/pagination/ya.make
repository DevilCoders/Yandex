GO_LIBRARY()

OWNER(g:mdb)

SRCS(pagination.go)

GO_TEST_SRCS(pagination_test.go)

END()

RECURSE(gotest)
