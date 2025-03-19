GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(copy_to_local.go)

GO_TEST_SRCS(copy_to_local_test.go)

END()

RECURSE(gotest)
