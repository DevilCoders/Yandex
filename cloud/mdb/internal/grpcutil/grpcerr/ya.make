GO_LIBRARY()

OWNER(g:mdb)

SRCS(errors.go)

GO_TEST_SRCS(errors_test.go)

END()

RECURSE(gotest)
