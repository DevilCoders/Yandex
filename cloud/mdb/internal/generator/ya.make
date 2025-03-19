GO_LIBRARY()

OWNER(g:mdb)

SRCS(generator.go)

GO_TEST_SRCS(generator_test.go)

END()

RECURSE(gotest)
