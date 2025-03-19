GO_LIBRARY()

OWNER(g:mdb)

SRCS(config.go)

GO_TEST_SRCS(config_test.go)

END()

RECURSE(
    airflow
    clickhouse
    common
    console
    elasticsearch
    factory
    gotest
    greenplum
    internal
    kafka
    metastore
    mongodb
    mysql
    opensearch
    postgresql
    redis
    sqlserver
    support
)
