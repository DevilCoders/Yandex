GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    binlogs.go
    conn.go
    data.go
    queries.go
)

GO_TEST_SRCS(
    binlogs_test.go
    conn_test.go
)

END()

RECURSE(gotest)
