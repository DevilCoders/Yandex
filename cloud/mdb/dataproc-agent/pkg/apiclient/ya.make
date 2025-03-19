GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(apiclient.go)

END()

RECURSE(
    grpcclient
    mocks
)
