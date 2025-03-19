GO_LIBRARY()

OWNER(g:mdb)

SRCS(helpers.go)

GO_TEST_SRCS(helpers_test.go)

END()

RECURSE(gotest)
