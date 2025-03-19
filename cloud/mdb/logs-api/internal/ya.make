GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    app.go
    config.go
)

END()

RECURSE(
    api
    auth
    health
    logic
    logsdb
    models
)
