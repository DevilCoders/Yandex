GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    auth.go
    config.go
    methods.go
    models.go
)

END()

RECURSE(
    datacloudgrpc
    grpc
)
