GO_LIBRARY()

OWNER(g:mdb)

SRCS(vpcdb.go)

END()

RECURSE(
    mocks
    models
    pg
)
