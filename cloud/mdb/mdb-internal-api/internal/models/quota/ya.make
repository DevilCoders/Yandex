GO_LIBRARY()

OWNER(g:mdb)

SRCS(resources.go)

GO_TEST_SRCS(resources_test.go)

END()

RECURSE(gotest)
