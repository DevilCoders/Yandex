GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    base.go
    iteration.go
    worker.go
)

END()

RECURSE(config)
