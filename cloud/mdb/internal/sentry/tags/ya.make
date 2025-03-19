GO_LIBRARY()

OWNER(g:mdb)

SRCS(tags.go)

GO_TEST_SRCS(tags_test.go)

END()

RECURSE(gotest)
