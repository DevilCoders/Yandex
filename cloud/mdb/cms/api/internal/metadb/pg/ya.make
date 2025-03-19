GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clusters.go
    models.go
    pg.go
    tasks.go
)

END()

RECURSE(tests)
