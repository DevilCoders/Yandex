GO_LIBRARY()

OWNER(g:mdb)

SRCS(dbspecific.go)

GO_TEST_SRCS(dbspecific_test.go)

END()

RECURSE(
    clickhouse
    elasticsearch
    gotest
    greenplum
    kafka
    mongodb
    mysql
    postgresql
    redis
    sqlserver
    testhelpers
)
