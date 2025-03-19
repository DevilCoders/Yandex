GO_LIBRARY()

OWNER(g:mdb)

SRCS(client.go)

END()

RECURSE(
    mocks
    models
    provider
)
