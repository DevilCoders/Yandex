GO_LIBRARY()

OWNER(g:mdb)

SRCS(app.go)

END()

RECURSE(
    cms
    maintainer
    metadb
    models
    stat
)
