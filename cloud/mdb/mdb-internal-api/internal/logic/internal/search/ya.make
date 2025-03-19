GO_LIBRARY()

OWNER(g:mdb)

SRCS(search.go)

END()

RECURSE(
    mocks
    provider
)
