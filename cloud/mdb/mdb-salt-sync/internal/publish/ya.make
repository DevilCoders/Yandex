GO_LIBRARY()

OWNER(g:mdb)

SRCS(publish.go)

GO_TEST_SRCS(publish_test.go)

END()

RECURSE(gotest)
