GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    helpers.go
    resmanager.go
)

END()

RECURSE(
    cmd
    grpc
    http
    mocks
)
