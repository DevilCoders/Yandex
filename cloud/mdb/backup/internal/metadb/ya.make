GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    errors.go
    metadb.go
)

END()

RECURSE(
    mocks
    pg
)
