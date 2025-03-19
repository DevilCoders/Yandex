GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    planner.go
)

GO_XTEST_SRCS(
    cluster_test.go
    planner_test.go
)

END()

RECURSE(
    clickhouse
    gotest
    greenplum
    kafka
    mongodb
    mysql
    postgresql
    redis
    sqlserver
)
