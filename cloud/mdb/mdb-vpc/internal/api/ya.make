GO_LIBRARY()

OWNER(g:mdb)

SRCS(app.go)

END()

RECURSE(
    auth
    config
    console
    health
    network
    networkconnection
    validation
)
