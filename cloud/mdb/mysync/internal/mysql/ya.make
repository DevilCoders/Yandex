GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    commands.go
    data.go
    node.go
    queries.go
    util.go
)

END()

RECURSE(gtids)
