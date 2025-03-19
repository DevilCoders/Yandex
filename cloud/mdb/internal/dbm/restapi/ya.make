GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    allow_new_hosts.go
    client.go
    containers.go
    protocol.go
    volumes.go
)

GO_TEST_SRCS(protocol_test.go)

END()

RECURSE(
    cli
    gotest
)
