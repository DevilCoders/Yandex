GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    app.go
    configs.go
    logging.go
)

END()

RECURSE(
    config
)
