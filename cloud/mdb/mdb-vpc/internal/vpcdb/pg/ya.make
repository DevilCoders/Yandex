GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    network_connections.go
    networks.go
    operations.go
    pg.go
    tx.go
)

END()

RECURSE(
    internal
    tests
)
