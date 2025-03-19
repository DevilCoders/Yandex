GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clusterservice.go
    errors.go
    field_paths.go
    models.go
    saltenvmapper.go
)

GO_TEST_SRCS(
    clusterservice_test.go
    errors_test.go
    field_paths_test.go
    models_test.go
)

END()

RECURSE(
    airflow
    clickhouse
    common
    console
    elasticsearch
    gotest
    greenplum
    kafka
    metastore
    mocks
    mongodb
    mysql
    opensearch
    postgresql
    redis
    sqlserver
    support
)
