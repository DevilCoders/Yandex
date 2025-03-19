GO_LIBRARY()

OWNER(g:mdb)

SRCS(console.go)

END()

RECURSE(
    mocks
    provider
)
