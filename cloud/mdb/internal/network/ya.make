GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    network.go
    subnet.go
)

END()

RECURSE(
    doublecloud
    meta
    mocks
    nop
    porto
)
