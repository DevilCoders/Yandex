GO_LIBRARY()

OWNER(g:mdb)

SRCS(greenplum.go)

GO_TEST_SRCS(greenplum_test.go)

END()

RECURSE(gotest)
