GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    billing_private.go
    grpc.go
    load.go
    logging.go
    marketplace_private.go
    service.go
    worker.go
    ydb.go
)

END()
