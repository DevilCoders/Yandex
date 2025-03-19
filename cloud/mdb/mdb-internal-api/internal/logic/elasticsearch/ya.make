GO_LIBRARY()

OWNER(g:mdb)

SRCS(elasticsearch.go)

END()

RECURSE(
    esmodels
    mocks
    provider
)
