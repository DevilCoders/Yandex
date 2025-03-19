GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    config.go
    interval.go
    sslmode.go
)

END()

RECURSE(
    pgerrors
    recipeconfig
)
