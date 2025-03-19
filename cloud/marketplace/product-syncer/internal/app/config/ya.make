GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    yc.go
    load.go
    logging.go
    marketplace_private.go
    s3.go
    service.go
    tracer.go
)

END()
