GO_LIBRARY()

OWNER(g:mdb)

SRCS(logsdb.go)

END()

RECURSE(
    clickhouse
    file
    mocks
)
