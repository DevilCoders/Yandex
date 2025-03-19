GO_LIBRARY()

OWNER(g:mdb)

SRCS(greenplum.go)

END()

RECURSE(
    gpmodels
    mocks
    provider
)
