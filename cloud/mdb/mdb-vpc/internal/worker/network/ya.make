GO_LIBRARY()

OWNER(g:mdb)

SRCS(network.go)

END()

RECURSE(
    aws
    mocks
)
