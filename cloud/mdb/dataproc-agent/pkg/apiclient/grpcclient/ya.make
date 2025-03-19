GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(
    grpcclient.go
    jobs.go
)

GO_TEST_SRCS(grpcclient_test.go)

END()

RECURSE(gotest)
