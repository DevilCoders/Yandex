GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    sqlcollations.go
    sqlserver.go
)

END()

RECURSE(
    mocks
    provider
    ssmodels
)
