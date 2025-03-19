GO_LIBRARY()

OWNER(g:mdb)

SRCS(health.go)

END()

RECURSE(
    mocks
    provider
)
