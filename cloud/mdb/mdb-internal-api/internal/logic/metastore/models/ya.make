GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    config.go
    constants.go
    operation.go
    tasks.go
    version.go
)

END()

RECURSE(gotest)
