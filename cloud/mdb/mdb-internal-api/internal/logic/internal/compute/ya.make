GO_LIBRARY()

OWNER(g:mdb)

SRCS(compute.go)

END()

RECURSE(
    mocks
    provider
)
