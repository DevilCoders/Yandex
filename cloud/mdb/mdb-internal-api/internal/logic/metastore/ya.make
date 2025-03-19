GO_LIBRARY()

OWNER(g:mdb)

SRCS(metastore.go)

END()

RECURSE(
    mocks
    models
    provider
)
