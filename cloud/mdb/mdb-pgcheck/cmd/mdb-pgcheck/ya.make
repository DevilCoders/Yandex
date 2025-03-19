GO_PROGRAM()

OWNER(g:mdb)

SRCS(
    config.go
    daemon.go
    database.go
    host.go
    http.go
    pgcheck.go
    priority.go
)

GO_TEST_SRCS(
    http_test.go
    pgcheck_test.go
    priority_test.go
)

END()

RECURSE(gotest)
