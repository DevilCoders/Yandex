GO_LIBRARY()

OWNER(
    g:mdb-dataproc
    g:mdb
)

SRCS(datastore.go)

END()

RECURSE(
    mocks
    redis
)
