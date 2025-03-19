GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    mongodb.go
    perfdiag.go
)

END()

RECURSE(
    mocks
    mongomodels
    perfdiagdb
    provider
)
