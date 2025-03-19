GO_LIBRARY()

OWNER(g:mdb)

SRCS(metastore.go)

GO_TEST_SRCS(metastore_test.go)

END()

RECURSE(gotest)
