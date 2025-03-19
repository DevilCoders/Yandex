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
    crypto
    health
    logic
    metadb
)
