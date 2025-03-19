GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    generate.go
    perfdiag.go
    postgresql.go
)

END()

RECURSE(
    mocks
    perfdiagdb
    pgmodels
    provider
)
