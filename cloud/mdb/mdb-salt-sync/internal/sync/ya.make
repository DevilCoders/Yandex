GO_LIBRARY()

OWNER(g:mdb)

SRCS(sync.go)

GO_TEST_SRCS(sync_test.go)

END()

RECURSE(gotest)
