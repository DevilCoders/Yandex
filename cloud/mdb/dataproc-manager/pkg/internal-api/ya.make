GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(client.go)

END()

RECURSE(
    http
    mocks
)
