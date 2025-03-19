GO_LIBRARY()

OWNER(g:mdb)

SRCS(attributes.go)

GO_TEST_SRCS(attributes_test.go)

END()

RECURSE(gotest)
