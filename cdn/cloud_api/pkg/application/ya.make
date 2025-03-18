GO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

SRCS(
    app.go
    grpc_server.go
    http_server.go
)

END()

RECURSE(
    closer
    xmiddleware
)
