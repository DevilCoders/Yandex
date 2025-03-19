GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    context.go
    query.go
    query_context.go
    tx.go
)

GO_XTEST_SRCS(query_test.go)

END()

RECURSE(
    chutil
    functest
    gotest
    pgutil
    sqlerrors
    tracing
)
