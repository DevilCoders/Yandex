GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    macro.go
    owners.go
)

END()

RECURSE(
    gotest
    http
    mocks
    nop
)
