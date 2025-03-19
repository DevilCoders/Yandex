GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    aggregate_info.go
    cluster_health.go
    cluster_secret.go
    config.go
    host_health.go
    host_neighbours.go
    lua.go
    marshal.go
    redis.go
    stats.go
    topology.go
    types.go
    unhealthy_aggregated_info.go
    unmarshal.go
)

GO_XTEST_SRCS(
    cluster_health_test.go
    cluster_secret_test.go
    dbspecific_clickhouse_test.go
    dbspecific_elasticsearch_test.go
    dbspecific_kafka_test.go
    dbspecific_mongo_test.go
    dbspecific_mysql_test.go
    dbspecific_postgresql_test.go
    dbspecific_redis_test.go
    host_health_test.go
    redis_bb_test.go
    redis_unit_test.go
    unhealthy_aggregated_info_test.go
)

END()

RECURSE(gotest)
