GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    codes.go
    serve.go
)

END()

RECURSE(
    gotest
    grpcerr
    grpcmocker
    interceptors
    internal
    optional
)
