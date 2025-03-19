GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    app.go
    configs.go
    logging.go
    worker.go
)

END()

RECURSE(config)
