GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    config.go
    decisions.go
    instances.go
    locks.go
    pg.go
    statistics.go
    tasks.go
    tx.go
)

END()

RECURSE(
    gotest
    internal
    tests
)
