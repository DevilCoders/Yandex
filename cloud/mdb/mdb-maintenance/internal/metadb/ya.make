GO_LIBRARY()

OWNER(g:mdb)

SRCS(metadb.go)

END()

RECURSE(
    mocks
    pg
)
