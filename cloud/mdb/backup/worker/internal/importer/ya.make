GO_LIBRARY()

OWNER(g:mdb)

SRCS(importer.go)

END()

RECURSE(
    clickhouse
    mongodb
    mysql
    postgresql
)
