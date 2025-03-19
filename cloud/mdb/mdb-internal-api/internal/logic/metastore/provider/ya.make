GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clusters.go
    console.go
    create.go
    metastore.go
    utils.go
)

END()

RECURSE(
    gotest
    internal
)
