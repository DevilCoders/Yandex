GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    airflow.go
    clusters.go
    create.go
    delete.go
    utils.go
)

END()

RECURSE(
    gotest
    internal
)
