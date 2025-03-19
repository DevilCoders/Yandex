GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    access_service.go
    billing_private.go
    grpc.go
    http.go
    load.go
    logging.go
    monitoring.go
    resource_manager.go
    service.go
    tracer.go
    ydb.go
)

END()
