GO_LIBRARY()

OWNER(g:mdb)

SRCS(app.go)

END()

RECURSE(
    api
    core
    deploydb
)
