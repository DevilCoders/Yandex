GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    volumes.go
)

END()

RECURSE(
    mocks
    restapi
)
