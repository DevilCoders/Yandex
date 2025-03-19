GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(decommission.go)

GO_TEST_SRCS(decommission_test.go)

END()

RECURSE(gotest)
