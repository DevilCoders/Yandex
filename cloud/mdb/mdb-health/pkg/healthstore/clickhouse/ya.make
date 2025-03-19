GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    backend.go
    host_health.go
    sql.go
)

GO_XTEST_SRCS(
    backend_test.go
    host_health_test.go
)

END()

RECURSE(
    gotest
    internal
)
