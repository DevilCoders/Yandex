GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    metadb.go
    models.go
)

END()

RECURSE(
    mocks
    pg
)
