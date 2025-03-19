GO_LIBRARY()

OWNER(g:mdb)

SRCS(metadata.go)

GO_TEST_SRCS(metadata_test.go)

END()

RECURSE(gotest)
