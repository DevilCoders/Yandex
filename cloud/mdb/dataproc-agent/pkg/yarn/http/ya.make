GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(client.go)

GO_TEST_SRCS(client_test.go)

END()

RECURSE(gotest)
