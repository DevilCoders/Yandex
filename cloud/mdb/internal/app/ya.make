GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    app.go
    config.go
    init.go
    logging.go
    options.go
)

END()

RECURSE(
    environment
    gotest
    signals
    swagger
)
