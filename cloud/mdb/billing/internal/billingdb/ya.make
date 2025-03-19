GO_LIBRARY()

OWNER(g:mdb)

SRCS(billingdb.go)

END()

RECURSE(
    mocks
    pg
)
