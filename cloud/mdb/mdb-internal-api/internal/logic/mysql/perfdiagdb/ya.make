GO_LIBRARY()

OWNER(g:mdb)

SRCS(perfdiagdb.go)

END()

RECURSE(
    clickhouse
    mocks
)
