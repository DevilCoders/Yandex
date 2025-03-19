GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    network.go
    network_connetion.go
    operation.go
)

END()

RECURSE(aws)
