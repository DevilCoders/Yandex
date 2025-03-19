GO_LIBRARY()

OWNER(g:mdb)

SRCS(metadb.go)

GO_TEST_SRCS(metadb_test.go)

END()

RECURSE(gotest)
