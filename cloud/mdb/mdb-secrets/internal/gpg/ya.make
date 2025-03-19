GO_LIBRARY()

OWNER(g:mdb)

SRCS(generate.go)

GO_TEST_SRCS(generate_test.go)

END()

RECURSE(gotest)
