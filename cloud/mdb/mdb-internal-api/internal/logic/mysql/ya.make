GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    mysql.go
    perfdiag.go
)

END()

RECURSE(
    mocks
    mymodels
    perfdiagdb
    provider
)
