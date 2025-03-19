GO_LIBRARY()

OWNER(g:mdb)

SRCS(opensearch.go)

END()

RECURSE(
    mocks
    osmodels
    provider
)
